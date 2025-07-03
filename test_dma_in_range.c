
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned long long dma_addr_t;
#define TRBS_PER_SEGMENT 256
#define TRB_SEGMENT_SIZE (TRBS_PER_SEGMENT*16)
#define TRB_SEGMENT_SHIFT	(9)

struct xhci_trb {
	int field[4];
};

struct xhci_segment {
	struct xhci_segment *next;
	struct xhci_trb *trbs;
	dma_addr_t dma;
	int num;
};

#define in_range(val, start, len) ((val - start) < len)

/* Create a new struct xhci_segment and alloc its TRBs */
struct xhci_segment *new_seg(int num)
{
	struct xhci_segment *seg;

	seg = malloc(sizeof(struct xhci_segment));
	if (seg) {
		seg->dma = (dma_addr_t)malloc(TRB_SEGMENT_SIZE);
		if (seg->dma) {
			seg->next = NULL;
			seg->num = num;
			seg->trbs = (struct xhci_trb *)(seg->dma);
			return seg;
		}
	}
	return NULL;
}

/* Same as xhci's, except it does not translate trb to dma */
dma_addr_t xhci_trb_virt_to_dma(struct xhci_segment *seg, struct xhci_trb *trb)
{
	unsigned long segment_offset;

	if (!seg || !trb || trb < seg->trbs)
		return 0;

	segment_offset = trb - seg->trbs;
	if (segment_offset >= TRBS_PER_SEGMENT)
		return 0;
	return seg->dma + (segment_offset * sizeof(*trb));
}

/* Same as xhci's, except it does not translate dma to trb */
static struct xhci_trb *xhci_dma_to_virt_trb(struct xhci_segment *seg, dma_addr_t dma)
{
	unsigned long trb_offset;

	if (dma < (dma_addr_t)seg->trbs)
		return NULL;

	trb_offset = (dma - (dma_addr_t)seg->trbs) / sizeof(struct xhci_trb);
	if (trb_offset >= TRBS_PER_SEGMENT)
		return NULL;
	return &seg->trbs[trb_offset];
}

static struct xhci_segment *old_dma_in_range(struct xhci_segment *start_seg, struct xhci_trb *start_trb,
					  struct xhci_segment *end_seg, struct xhci_trb *end_trb,
					  dma_addr_t dma)
{
	dma_addr_t start_dma;
	dma_addr_t end_seg_dma;
	dma_addr_t end_trb_dma;
	struct xhci_segment *cur_seg;

	start_dma = xhci_trb_virt_to_dma(start_seg, start_trb);
	cur_seg = start_seg;

	do {
		if (start_dma == 0)
			return NULL;
		/* We may get an event for a Link TRB in the middle of a TD */
		end_seg_dma = xhci_trb_virt_to_dma(cur_seg,
				&cur_seg->trbs[TRBS_PER_SEGMENT - 1]);
		/* If the end TRB isn't in this segment, this is set to 0 */
		end_trb_dma = xhci_trb_virt_to_dma(cur_seg, end_trb);

		if (end_trb_dma > 0) {
			/* The end TRB is in this segment, so suspect should be here */
			if (start_dma <= end_trb_dma) {
				if (dma >= start_dma && dma <= end_trb_dma)
					return cur_seg;
			} else {
				/* Case for one segment with
				 * a TD wrapped around to the top
				 */
				if ((dma >= start_dma && dma <= end_seg_dma) ||
				    (dma >= (dma_addr_t)cur_seg->trbs && dma <= end_trb_dma))
					return cur_seg;
			}
			return NULL;
		}
		/* Might still be somewhere in this segment */
		if (dma >= start_dma && dma <= end_seg_dma)
			return cur_seg;

		cur_seg = cur_seg->next;
		start_dma = xhci_trb_virt_to_dma(cur_seg, &cur_seg->trbs[0]);
	} while (cur_seg != start_seg);

	return NULL;
}

/*
 * 1.  [Start -> TRB -> End]
 * 2.  [End - Start -> TRB]
 * 3.  [Start -> End - TRB]
 * 4.  [TRB -> End - Start]
 * 5.  [End - TRB - Start]
 * 6.  [TRB - Start -> End]
 * 
 * 7.  [Start -> TRB]
 * 8.  [Start -> End]
 * 9.  [End - Start]
 * 10. [End - TRB]
 * 11. [TRB -> End]
 * 12. [TRB - Start]
 * 
 * 13. [Start] -> [End]
 * 14. [Start] -> [TRB] -> [End]
 * 15. [End] - [TRB] - [Start]
 */
static struct xhci_segment *new_dma_in_range(struct xhci_segment *start_seg, struct xhci_trb *start_trb,
					 struct xhci_segment *end_seg, struct xhci_trb *end_trb,
					 dma_addr_t dma)
{
	struct xhci_segment *seg;
	struct xhci_trb *trb;

	/* Added for legacy reasons. This should never be the case */
	if (!start_seg || !start_trb ||
	    (start_trb - start_seg->trbs) >= TRBS_PER_SEGMENT)
		return NULL;

	/* 1. Check if the DMA is after start TRB in start segment */
	trb = xhci_dma_to_virt_trb(start_seg, dma);
	if (start_trb <= trb) {
                // 1, 2, 3, 7
		if (end_seg != start_seg || trb <= end_trb)
			return start_seg; // 7, 1
		if (end_trb < start_trb)
			return start_seg; // 2
		return NULL; // 3
	}
        // 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15
 
        /* 2. Check if the DMA is before End TRB in end segment */
	if (start_seg != end_seg) {
                // 10, 11, 12, 13, 14, 15
                if (trb)
                        return NULL; // 12
                trb = xhci_dma_to_virt_trb(end_seg, dma);
	} else if (start_trb <= end_trb) {
		return NULL; // 6, 8
	} // else 4, 5, 9

        // 4, 5, 9, 10, 11, 13, 14, 15
	if (trb)// 4, 5, 10, 11
		return (trb <= end_trb) ? end_seg : NULL;

        // 9, 13, 14, 15
	/* 3. Check segments between Start and End */
	for (seg = start_seg->next; seg != end_seg && seg != start_seg; seg = seg->next) {
		if (in_range(dma, seg->dma, TRB_SEGMENT_SIZE))
			return seg;
	}

	return NULL;
}

static unsigned int trb_to_pos(struct xhci_segment *seg, struct xhci_trb *trb)
{
	return (seg->num << TRB_SEGMENT_SHIFT) + (trb - seg->trbs);
}

struct xhci_segment *mat_dma_in_range(struct xhci_segment *start_seg, struct xhci_trb *start_trb,
			              struct xhci_segment *end_seg, struct xhci_trb *end_trb,
                                      dma_addr_t dma)
{
	unsigned int val, start, end;
	struct xhci_segment *seg;

	if (!start_seg || !end_seg)
                return NULL;

        seg = start_seg;
        /* Loop through segments until the DMA address is found */
        while (!in_range(dma, seg->dma, TRB_SEGMENT_SIZE)) {
                seg = seg->next;
                /* Loop entire ring because of wrap case */
                if (seg == start_seg)
                        return NULL;
        }

	val = (seg->num << TRB_SEGMENT_SHIFT) +
              (dma - seg->dma) / sizeof(struct xhci_trb);
	start = trb_to_pos(start_seg, start_trb);
	end = trb_to_pos(end_seg, end_trb);

	/* Range is wrapped */
	if (end < start) {
		if (val > end && val < start)
                        return NULL;
                return seg;
        }

	if (start <= val && val <= end)
                return seg;
        return NULL;
}

static unsigned int pos_to_idx(unsigned int pos, unsigned int trbs_in_ring, unsigned int min_idx)
{
        return (pos < min_idx) ? pos + trbs_in_ring : pos;
}

struct xhci_segment *idx_dma_in_range(struct xhci_segment *start_seg, struct xhci_trb *start_trb,
			              struct xhci_segment *end_seg, struct xhci_trb *end_trb,
                                      dma_addr_t dma)
{
	unsigned int val, start, end;
	struct xhci_segment *seg;

	if (!start_seg || !end_seg)
                return NULL;

        seg = start_seg;
        /* Loop through segments until the DMA address is found */
        while (!in_range(dma, seg->dma, TRB_SEGMENT_SIZE)) {
                seg = seg->next;
                /* Loop entire ring because of wrap case */
                if (seg == start_seg)
                        return NULL;
        }

	val = (seg->num << TRB_SEGMENT_SHIFT) +
              (dma - seg->dma) / sizeof(struct xhci_trb);


        start = trb_to_pos(start_seg, start_trb);
	if (val < start)
                return NULL;

        end = trb_to_pos(end_seg, end_trb);
        end = pos_to_idx(end, 1536, start);
	if (end < val)
                return NULL;

        return seg;
}


#define func1	idx_dma_in_range
#define func2	mat_dma_in_range
#define DMA_IN_RANGE(seg1, trb1, seg2, trb2, segX, trbX)	\
	func1(seg1, &(seg1)->trbs[trb1], seg2, &(seg2)->trbs[trb2], (dma_addr_t)(&(segX)->trbs[trbX]))

int main()
{
	int addr;
	struct xhci_segment *seg1 = new_seg(1);
	seg1->next = seg1;

	if (seg1 != DMA_IN_RANGE(seg1, 0, seg1, 100, seg1, 50))
		printf("ERROR: 1. [Start -> TRB -> End]\n");
	if (seg1 != DMA_IN_RANGE(seg1, 50, seg1, 0, seg1, 100))
		printf("ERROR: 2. [End - Start -> TRB]\n");
	if (NULL != DMA_IN_RANGE(seg1, 0, seg1, 50, seg1, 100))
		printf("ERROR: 3. [Start -> End - TRB]\n");
	if (seg1 != DMA_IN_RANGE(seg1, 100, seg1, 50, seg1, 0))
		printf("ERROR: 4. [TRB -> End - Start]\n");
	if (NULL != DMA_IN_RANGE(seg1, 100, seg1, 0, seg1, 50))
		printf("ERROR: 5. [End - TRB - Start]\n");
	if (NULL != DMA_IN_RANGE(seg1, 50, seg1, 100, seg1, 0))
		printf("ERROR: 6. [TRB - Start -> End]\n");

	struct xhci_segment *seg3 = new_seg(3);
	struct xhci_segment *seg2 = new_seg(2);
	seg1->next = seg2;
	seg2->next = seg3;
	seg3->next = seg1;

	if (seg1 != DMA_IN_RANGE(seg1, 0, seg2, 0, seg1, 100))
		printf("ERROR: 7. [Start -> TRB]\n");
        if (NULL != DMA_IN_RANGE(seg1, 0, seg1, 100, seg2, 0))
		printf("ERROR: 8. [Start -> End]\n");
	if (seg2 != DMA_IN_RANGE(seg1, 100, seg1, 0, seg2, 100))
		printf("ERROR: 9. [End - Start]\n"); // Old fails   
	if (NULL != DMA_IN_RANGE(seg1, 0, seg2, 0, seg2, 100))
		printf("ERROR: 10. [End - TRB]\n");
	if (seg2 != DMA_IN_RANGE(seg1, 0, seg2, 100, seg2, 0))
		printf("ERROR: 11. [TRB -> End]\n");
	if (NULL != DMA_IN_RANGE(seg1, 100, seg3, 0, seg1, 0))
		printf("ERROR: 12. [TRB - Start]\n");

        if (NULL != DMA_IN_RANGE(seg1, 0, seg2, 100, seg3, 0))
		printf("ERROR: 13. [Start] -> [End]\n");
	if (seg2 != DMA_IN_RANGE(seg1, 0, seg3, 0, seg2, 0))
		printf("ERROR: 14. [Start] -> [TRB] -> [End]\n");
        if (NULL != DMA_IN_RANGE(seg1, 0, seg2, 0, seg3, 0))
		printf("ERROR: 15. [Start] -> [End] - [TRB]\n");


	/* Wrong input */
	if (NULL != func1(seg1, &seg3->trbs[0], seg1, &seg1->trbs[100], (dma_addr_t)&seg1->trbs[50]))
		printf("ERROR: Start TRB not in Start segment\n");
        if (NULL != func1(seg1, &seg1->trbs[0], seg1, &seg3->trbs[100], (dma_addr_t)&seg1->trbs[50]))
		printf("WARN: End TRB not in End segment\n"); // Old & New fails
	if (NULL != func1(seg1, &seg1->trbs[0], seg2, &seg2->trbs[0], (dma_addr_t)&addr))
		printf("ERROR: DMA is not a TRB\n");

	/* Missing input */
	if (NULL != func1(NULL, &seg1->trbs[0], seg2, &seg2->trbs[0], (dma_addr_t)&seg1->trbs[100]))
		printf("ERROR: Start segment is NULL\n");
	if (NULL != func1(seg1, NULL, seg2, &seg2->trbs[0], (dma_addr_t)&seg1->trbs[100]))
		printf("ERROR: Start TRB is NULL\n");
	if (NULL != func1(seg1, &seg1->trbs[0], NULL, &seg2->trbs[0], (dma_addr_t)&seg1->trbs[100]))
		printf("WARN: End segment is NULL\n"); // Old & New fails
	if (NULL != func1(seg1, &seg1->trbs[0], seg2, NULL, (dma_addr_t)&seg1->trbs[100]))
		printf("WARN: End TRB is NULL\n"); // Old & New fails
	if (NULL != func1(seg1, &seg1->trbs[0], seg2, &seg2->trbs[0], 0))
		printf("ERROR: TRB is NULL\n");

	struct xhci_segment *seg4 = new_seg(4);
	struct xhci_segment *seg5 = new_seg(5);
        struct xhci_segment *seg6 = new_seg(6);
	seg3->next = seg4;
        seg4->next = seg5;
        seg5->next = seg6;
        seg6->next = seg1;

	int i;
	int s, e, x;
	struct xhci_segment *res1, *res2;
	struct xhci_segment *start, *end, *target;
	struct xhci_segment *segments[6] = {seg1, seg2, seg3, seg4, seg5, seg6};
	// for (i = 0; i < 100000; i++) {
	// 	/* Single segment test with random TRBs */
	// 	s = rand() % (TRBS_PER_SEGMENT - 1);
	// 	e = rand() % (TRBS_PER_SEGMENT - 1);
	// 	x = rand() % (TRBS_PER_SEGMENT - 1);
	// 	res1 = func1(seg1, &seg1->trbs[s], seg1, &seg1->trbs[e], (dma_addr_t)&seg1->trbs[x]);
	// 	res2 = func2(seg1, &seg1->trbs[s], seg1, &seg1->trbs[e], (dma_addr_t)&seg1->trbs[x]);
	// 	if (res1 != res2) {
	// 		printf("\nMismatch between new and old dma_in_range():\n");
	// 		printf("  seg %d != %d\n", (res1 ? res1->num : 0), (!res2 ? 0 : res2->num));
	// 		printf("  1[%d] -> 1[%d] dma: [%d]\n", s, e, x);
	// 		break;
	// 	}

	// 	/* Triple segment test with random segments and TRBs */
	// 	start = segments[rand() % 3];
	// 	end = segments[rand() % 3];
	// 	target = segments[rand() % 3];
	// 	res1 = func1(start, &start->trbs[s], end, &end->trbs[e], (dma_addr_t)&target->trbs[x]);
	// 	res2 = func2(start, &start->trbs[s], end, &end->trbs[e], (dma_addr_t)&target->trbs[x]);
	// 	if (res1 != res2) {
	// 		/* Case which old_dma_in_range() can't handle */
	// 		if (res2 == NULL && start == end && start != target && e < s)
	// 			continue;
	// 		printf("\nMismatch between new and old dma_in_range():\n");
	// 		printf("  seg %d != %d\n", (res1 ? res1->num : 0), (res2 ? res2->num : 0));
	// 		printf("  %d[%d] -> %d[%d] dma: %d[%d]\n", start->num, s, end->num, e, target->num, x);
	// 		break;
	// 	}
	// }

        clock_t begin, total1, total2;
        total1 = 0;
        total2 = 0;
	for (i = 0; i < 100000; i++) {
		/* Single segment test with random TRBs */
		s = rand() % (TRBS_PER_SEGMENT - 1);
		e = rand() % (TRBS_PER_SEGMENT - 1);
		x = rand() % (TRBS_PER_SEGMENT - 1);
		start = segments[rand() % 3];
		end = segments[rand() % 3];
		target = segments[rand() % 3];

                begin = clock();
                for (int j = 0; j < 10; j++) {
		        res1 = func1(seg1, &seg1->trbs[s], seg1, &seg1->trbs[e], (dma_addr_t)&seg1->trbs[x]);
                        res1 = func1(seg1, &seg1->trbs[x], seg1, &seg1->trbs[s], (dma_addr_t)&seg1->trbs[e]);
                        res1 = func1(seg1, &seg1->trbs[e], seg1, &seg1->trbs[x], (dma_addr_t)&seg1->trbs[s]);

		        res1 = func1(start, &start->trbs[s], end, &end->trbs[e], (dma_addr_t)&target->trbs[x]);
                        res1 = func1(start, &target->trbs[s], end, &start->trbs[e], (dma_addr_t)&end->trbs[x]);
                        res1 = func1(start, &end->trbs[s], end, &target->trbs[e], (dma_addr_t)&start->trbs[x]);
                }
                total1 += clock() - begin;

                begin = clock();
                for (int j = 0; j < 10; j++) {
		        res2 = func2(seg1, &seg1->trbs[s], seg1, &seg1->trbs[e], (dma_addr_t)&seg1->trbs[x]);
                        res2 = func2(seg1, &seg1->trbs[x], seg1, &seg1->trbs[s], (dma_addr_t)&seg1->trbs[e]);
                        res2 = func2(seg1, &seg1->trbs[e], seg1, &seg1->trbs[x], (dma_addr_t)&seg1->trbs[s]);

		        res2 = func2(start, &start->trbs[s], end, &end->trbs[e], (dma_addr_t)&target->trbs[x]);
                        res2 = func2(start, &target->trbs[s], end, &start->trbs[e], (dma_addr_t)&end->trbs[x]);
                        res2 = func2(start, &end->trbs[s], end, &target->trbs[e], (dma_addr_t)&start->trbs[x]);
                }
                total2 += clock() - begin;

	}
	printf("\nCompleted %d random tests in %d %d = %d\n", i, total1, total2, total1 - total2);

	free((void *)seg1->dma);
	free(seg1);
	free((void *)seg2->dma);
	free(seg2);
	free((void *)seg3->dma);
	free(seg3);

	return 0;
}