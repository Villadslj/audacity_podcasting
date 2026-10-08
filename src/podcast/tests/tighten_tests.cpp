/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/speechdetectioncore.h"

using namespace au::podcast;
using namespace au::podcast::core;

namespace {
void expectSpans(const TimeSpanList& actual, const std::vector<std::pair<double, double> >& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_NEAR(actual[i].start().to_double(), expected[i].first, 1e-9) << "span " << i;
        EXPECT_NEAR(actual[i].end().to_double(), expected[i].second, 1e-9) << "span " << i;
    }
}
}

TEST(PodcastTightenTests, ComplementOfSpeechRanges)
{
    TimeSpanList speech;
    speech.emplace_back(1.0, 2.0);
    speech.emplace_back(3.0, 4.0);

    expectSpans(complement(speech, 0.0, 5.0), { { 0.0, 1.0 }, { 2.0, 3.0 }, { 4.0, 5.0 } });
}

TEST(PodcastTightenTests, ComplementClampsToTheProcessedRange)
{
    TimeSpanList speech;
    speech.emplace_back(0.0, 2.0);
    speech.emplace_back(3.0, 10.0);

    expectSpans(complement(speech, 1.0, 5.0), { { 2.0, 3.0 } });
}

TEST(PodcastTightenTests, ComplementOfAnEmptyListIsTheWholeRange)
{
    expectSpans(complement({}, 1.0, 5.0), { { 1.0, 5.0 } });
    EXPECT_TRUE(complement({}, 5.0, 5.0).empty());
}

TEST(PodcastTightenTests, IntersectionOfTwoLists)
{
    TimeSpanList a;
    a.emplace_back(0.0, 2.0);
    a.emplace_back(3.0, 6.0);

    TimeSpanList b;
    b.emplace_back(1.0, 4.0);
    b.emplace_back(5.0, 7.0);

    expectSpans(intersect(a, b), { { 1.0, 2.0 }, { 3.0, 4.0 }, { 5.0, 6.0 } });
}

TEST(PodcastTightenTests, IntersectionOfDisjointListsIsEmpty)
{
    TimeSpanList a;
    a.emplace_back(0.0, 1.0);

    TimeSpanList b;
    b.emplace_back(2.0, 3.0);

    EXPECT_TRUE(intersect(a, b).empty());
}

TEST(PodcastTightenTests, IntersectionOfManyLists)
{
    TimeSpanList a;
    a.emplace_back(0.0, 10.0);

    TimeSpanList b;
    b.emplace_back(1.0, 5.0);
    b.emplace_back(6.0, 9.0);

    TimeSpanList c;
    c.emplace_back(2.0, 7.0);

    expectSpans(intersectAll({ a, b, c }), { { 2.0, 5.0 }, { 6.0, 7.0 } });
    EXPECT_TRUE(intersectAll({}).empty());
    expectSpans(intersectAll({ a }), { { 0.0, 10.0 } });
}

TEST(PodcastTightenTests, GapsShorterThanTheMinimumAreLeftAlone)
{
    TimeSpanList gaps;
    gaps.emplace_back(0.0, 0.2); // 200 ms

    TightenSettings settings;
    settings.minGapMs = 250.0;
    settings.percent = 30.0;

    EXPECT_TRUE(gapsToRemove(gaps, settings).empty());
}

TEST(PodcastTightenTests, RemovalIsTakenFromTheMiddleOfTheGap)
{
    TimeSpanList gaps;
    gaps.emplace_back(10.0, 11.0); // 1000 ms

    TightenSettings settings;
    settings.minGapMs = 250.0;
    settings.percent = 30.0;

    // 30 % of 1000 ms is 300 ms, which leaves 750 ms, so 300 ms are removed
    expectSpans(gapsToRemove(gaps, settings), { { 10.35, 10.65 } });
}

TEST(PodcastTightenTests, RemovalNeverShortensAGapBelowTheMinimum)
{
    TimeSpanList gaps;
    gaps.emplace_back(0.0, 0.3); // 300 ms

    TightenSettings settings;
    settings.minGapMs = 250.0;
    settings.percent = 30.0;

    // 30 % would be 90 ms, but only 50 ms may be removed
    expectSpans(gapsToRemove(gaps, settings), { { 0.125, 0.175 } });
}

TEST(PodcastTightenTests, RemovalsAreReturnedInAscendingOrderSoTheyCanBeAppliedBackwards)
{
    TimeSpanList gaps;
    gaps.emplace_back(1.0, 2.0);
    gaps.emplace_back(5.0, 7.0);

    TightenSettings settings;
    settings.minGapMs = 250.0;
    settings.percent = 50.0;

    const TimeSpanList removals = gapsToRemove(gaps, settings);

    expectSpans(removals, { { 1.25, 1.75 }, { 5.5, 6.5 } });

    // Walking the list backwards yields the removals from the end of the timeline to its
    // start, which is the order in which they have to be applied so that the positions
    // of the not yet applied removals stay valid.
    std::vector<double> starts;
    for (auto it = removals.rbegin(); it != removals.rend(); ++it) {
        starts.push_back(it->start().to_double());
    }

    ASSERT_EQ(starts.size(), 2u);
    EXPECT_GT(starts[0], starts[1]);
}

TEST(PodcastTightenTests, FramesToSpansMergesSpansThatOverlapAfterPadding)
{
    FrameRanges ranges;
    ranges.push_back({ 0, 10 });
    ranges.push_back({ 12, 20 });

    // 20 ms frames, 50 ms margin: the 40 ms gap disappears under the padding
    expectSpans(framesToSpans(ranges, 20.0, 0.0, 0.0, 10.0, 50.0), { { 0.0, 0.45 } });
}

TEST(PodcastTightenTests, FramesToSpansClampsToTheGivenBounds)
{
    FrameRanges ranges;
    ranges.push_back({ 0, 10 });

    expectSpans(framesToSpans(ranges, 20.0, 1.0, 1.0, 1.15, 50.0), { { 1.0, 1.15 } });
}
