#include "campaign.h"
#include <assert.h>
#include <math.h>

/* Campaign tick test: monthly payments are supplied by the economy snapshot.
 * They are no longer generated from a fixed tutorial number. */
int main(void)
{
    struct campaign campaign;
    campaign_start(&campaign);
    assert(campaign.treasury == 0 && campaign_monthly_income(&campaign) == 0);
    assert(campaign.date.year == 1066 && campaign.date.month == 9 && campaign.date.day == 15);
    campaign.treasury = 50;
    campaign.monthly_tax = 5.25;
    campaign.monthly_prestige = 0.2;
    campaign.monthly_piety = 0.1;
    assert(campaign_improve(&campaign));
    assert(campaign.treasury == 25 && campaign_monthly_income(&campaign) == 5.25);
    campaign_advance_month(&campaign);
    assert(campaign.date.month == 10 && campaign.date.day == 1 && campaign.treasury == 30);
    assert(fabs(campaign.gold_remainder - .25) < .0001);
    assert(fabs(campaign.prestige - .2) < .0001 && fabs(campaign.piety - .1) < .0001);
    campaign_advance_day(&campaign);
    assert(campaign.date.day == 2 && campaign.treasury == 30);
    for (int i = 0; i < 10; ++i) campaign_advance_month(&campaign);
    assert(campaign.date.year == 1067 && campaign.date.month == 8 && campaign.date.day == 1);
    assert(campaign.treasury == 82); /* 11 months * 5.25 = 57 whole gold */
    campaign.date = (struct campaign_date){1068, 2, 28};
    campaign_advance_day(&campaign);
    assert(campaign.date.month == 2 && campaign.date.day == 29);
    campaign_advance_day(&campaign);
    assert(campaign.date.month == 3 && campaign.date.day == 1 && campaign.treasury == 88);
    campaign.treasury = IMPROVEMENT_COST - 1;
    assert(!campaign_improve(&campaign) && campaign.improvements == 1);
    campaign.treasury = 100;
    assert(campaign_improve(&campaign));
    assert(campaign_improve(&campaign));
    assert(!campaign_improve(&campaign));
    assert(campaign.improvements == IMPROVEMENT_LIMIT && campaign.treasury == 50);
}
