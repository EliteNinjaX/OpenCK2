#include "campaign.h"

#include <assert.h>

int main(void)
{
    struct campaign campaign;
    campaign_start(&campaign);
    assert(campaign.treasury == 50 && campaign_monthly_income(&campaign) == 5);
    assert(campaign.date.year == 1066 && campaign.date.month == 9 && campaign.date.day == 15);

    assert(campaign_improve(&campaign));
    assert(campaign.treasury == 25 && campaign_monthly_income(&campaign) == 7);
    assert(!campaign_goal_met(&campaign));
    campaign_advance_month(&campaign);
    assert(campaign.date.month == 10 && campaign.date.day == 1 && campaign.treasury == 32);
    campaign_advance_day(&campaign);
    assert(campaign.date.day == 2 && campaign.treasury == 32);
    for (int i = 0; i < 10; ++i)
        campaign_advance_month(&campaign);
    assert(campaign.date.year == 1067 && campaign.date.month == 8 && campaign.date.day == 1);
    assert(campaign.treasury == 102 && campaign_goal_met(&campaign));

    campaign.date = (struct campaign_date){1068, 2, 28};
    campaign_advance_day(&campaign);
    assert(campaign.date.month == 2 && campaign.date.day == 29);
    campaign_advance_day(&campaign);
    assert(campaign.date.month == 3 && campaign.date.day == 1 && campaign.treasury == 109);

    campaign.treasury = IMPROVEMENT_COST - 1;
    assert(!campaign_improve(&campaign) && campaign.improvements == 1);
    campaign.treasury = 100;
    assert(campaign_improve(&campaign));
    assert(campaign_improve(&campaign));
    assert(!campaign_improve(&campaign));
    assert(campaign.improvements == IMPROVEMENT_LIMIT && campaign.treasury == 50);
}
