#include "../src/game/election_core.h"
#include "../src/game/law_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *name)
{
    FILE *f=fopen(name,"rb");if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) {fclose(f);return NULL;}
    long n=ftell(f);if (n<0 || n>6*1024*1024) {fclose(f);return NULL;}
    rewind(f);char *s=malloc((size_t)n+1);if (!s) {fclose(f);return NULL;}
    if (fread(s,1,(size_t)n,f)!=(size_t)n) {free(s);fclose(f);return NULL;}
    fclose(f);s[n]='\0';return s;
}
int main(int argc,char **argv)
{
    struct ck2_election_catalog votes={0};
    struct ck2_law_catalog laws={0};
    const char *synthetic=
        "byzantine_elective={ elector_selection={ days=-5 max_amount=8 "
        " additive_modifier={ value=30 FROMFROM={holder_scope={character=ROOT}} }} "
        " elector_vote_strength={days=5 additive_modifier={value=195}} "
        " candidate_vote_score={days=1 modifier={factor=2}} "
        " candidate_trigger={ NOT={character=FROM} FROM={OR={trait=brave}} }}";
    assert(ck2_elections_parse(&votes,synthetic));
    assert(votes.count==1);
    const struct ck2_election_rule *e=ck2_election_find(&votes,"byzantine_elective");
    assert(e && e->has_base_days[CK2_ELECTOR_SELECTION]);
    assert(e->base_days[CK2_ELECTOR_SELECTION]==-5);
    assert(e->has_max_electors && e->max_electors==8);
    assert(e->base_days[CK2_ELECTOR_VOTE_STRENGTH]==5);
    assert(e->modifier_count[CK2_ELECTOR_SELECTION]==1);
    assert(e->script[CK2_CANDIDATE_VOTE_SCORE] && ck2_election_excludes_incumbent(e));
    struct ck2_election_candidate incumbent={.ruler_id=100,.candidate_id=100};
    assert(ck2_election_candidate_trigger(e,&incumbent)==CK2_ELECTION_FALSE);
    struct ck2_election_candidate stranger={.ruler_id=100,.candidate_id=102};
    assert(ck2_election_candidate_trigger(e,&stranger)==CK2_ELECTION_UNKNOWN);
    /* Unknown nested NOT must not be mistaken for a top-level exclusion. */
    assert(ck2_elections_parse(&votes,"byzantine_elective={ candidate_trigger={FROM={NOT={character=FROM}}} }"));
    e=ck2_election_find(&votes,"byzantine_elective");
    assert(e && !ck2_election_excludes_incumbent(e));
    assert(!e->script[CK2_ELECTOR_SELECTION]); /* Complete mod override. */
    const char *law_text="succession_laws={succ_byzantine_elective={"
        "potential={succession={voting_rules=evil}}"
        "effect={other={succession={voting_rules=evil}} "
        "succession={base_type=feudal_elective voting_rules=byzantine_elective}}}}";
    assert(ck2_law_parse(&laws,law_text));
    const struct ck2_law *law=ck2_law_find(&laws,"succ_byzantine_elective");
    assert(law && !strcmp(law->election_base_type,"feudal_elective"));
    assert(!strcmp(law->election_voting_rules,"byzantine_elective"));
    ck2_law_catalog_free(&laws);
    ck2_elections_free(&votes);
    if (argc>1) {
        for (int i=1;i<argc;++i) {
            char *script=read_file(argv[i]);assert(script);
            assert(ck2_elections_parse(&votes,script));free(script);
        }
        e=ck2_election_find(&votes,"byzantine_elective");
        assert(e && e->script[CK2_CANDIDATE_TRIGGER]);
        assert(e->script[CK2_ELECTOR_SELECTION]);
        assert(e->script[CK2_ELECTOR_VOTE_STRENGTH]);
        assert(e->script[CK2_CANDIDATE_VOTE_SCORE]);
        assert(e->has_base_days[CK2_ELECTOR_SELECTION] && e->base_days[CK2_ELECTOR_SELECTION]==-5);
        assert(e->has_base_days[CK2_ELECTOR_VOTE_STRENGTH] && e->base_days[CK2_ELECTOR_VOTE_STRENGTH]==5);
        assert(ck2_election_excludes_incumbent(e));
        char offices[20][64]={{0}};
        const size_t office_count=ck2_election_office_keys(e,offices,20);
        assert(office_count>=7);
        bool found_marshal=false,found_commander=false;
        for (size_t i=0;i<office_count && i<20;++i) {
            if (!strcmp(offices[i],"job_marshal")) found_marshal=true;
            if (!strcmp(offices[i],"title_commander")) found_commander=true;
        }
        assert(found_marshal && found_commander);
        const char *banned[]={"blinded"};
        struct ck2_election_candidate invalid={.ruler_id=2,.candidate_id=3,
            .candidate_traits=banned,.candidate_trait_count=1};
        assert(ck2_election_candidate_trigger(e,&invalid)==CK2_ELECTION_FALSE);
        assert(ck2_election_candidate_trigger(e,&stranger)==CK2_ELECTION_UNKNOWN);
        printf("Indexed %zu original CK2 election rule sets; Byzantine sections and base scores verified\n",votes.count);
        ck2_elections_free(&votes);
    }
    puts("CK2 scripted election rule indexing, override, and law linkage passed");
    return 0;
}
