/*  test_classifier.c
 *
 *  Unit tests for classify_segments().
 *  Uses Unity (pulled in by Ceedling).
 */

#include "unity.h"
#include "ynvisible_7_segment.h"

/* ----------------------------------------------------------------------
 *  Helpers
 * --------------------------------------------------------------------*/
static void check_masks(uint8_t prev,
                        uint8_t next,
                        uint8_t expOn,
                        uint8_t expOff,
                        uint8_t expRefresh)
{
    uint8_t on, off, ref;
    classify_segments(prev, next, &on, &off, &ref);

    TEST_ASSERT_EQUAL_HEX8_MESSAGE(expOn,  on,  "switch-ON mask mismatch");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(expOff, off, "switch-OFF mask mismatch");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(expRefresh, ref, "refresh mask mismatch");
}

/* ----------------------------------------------------------------------
 *  Unity fixtures
 * --------------------------------------------------------------------*/
void setUp   (void) {}
void tearDown(void) {}

/* ----------------------------------------------------------------------
 *  Tests
 * --------------------------------------------------------------------*/

/* OFF → ON for a single segment (segment a) */
void test_SingleSegmentOn(void)
{
    /* prev = 0b0000 0000, next = 0b0000 0001 */
    check_masks(0x00, 0x01,
                /*exp*/ 0x01,   /* ON  */
                0x00,   /* OFF */
                0x00);  /* REF */
}

/* 1 → 8 : five new segments + refresh two */
void test_Digit1to8(void)
{
    /* 1 = 0b0000 0110,  8 = 0b0111 1111 */
    check_masks(0x06, 0x7F,
                /*exp ON */ 0x79,  /* a,d,e,f,g */
                /*exp OFF*/ 0x00,
                /*exp REF*/ 0x06); /* b,c */
}

/* Blank → 8 : all seven segments switch ON */
void test_BlankTo8(void)
{
    check_masks(0x00, 0x7F,
                /*exp*/ 0x7F,  /* ON 7 bits */
                0x00,
                0x00);
}

/* 8 → Blank : all seven segments switch OFF */
void test_8toBlank(void)
{
    check_masks(0x7F, 0x00,
                0x00,
                0x7F,   /* OFF 7 bits */
                0x00);
}

/* No change (3 refresh bits) */
void test_Digit7Refresh(void)
{
    /* 7 = 0b0000 0111 */
    check_masks(0x07, 0x07,
                0x00,
                0x00,
                0x07);   /* refresh a,b,c */
}
