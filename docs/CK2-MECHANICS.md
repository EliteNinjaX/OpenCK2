# Vanilla Crusader Kings II mechanics

This is a guide to the original game for contributors, including readers
without C experience. It describes mechanics at a level useful for planning
reference tests. The pinned baseline is CK2 3.3.5.1. Record the active DLC
and rules for each comparison, since they can change the available actions
and systems. The [implementation inventory](IMPLEMENTATION.md) describes
which parts OpenCK2 currently runs.

## The world the player controls

A campaign follows a ruler and their dynasty. Characters have family
relationships, culture, religion, traits, and claims. Titles identify
political positions from barony through empire. A title's current holder
and its place in the de facto vassal hierarchy can differ from its de jure
relationship. A map click can therefore identify a county holder whose top
liege is a different character. A county's capital and its other holdings
are separate seats. The ruler's demesne consists of holdings they control
directly. Vassals govern their own lands within the hierarchy. [Paradox CK2 overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/about)

The local installation provides starting conditions in
`common/bookmarks/`, `history/characters/`, `history/titles/`, and
`history/provinces/`. For example, `history/titles/k_france.txt` contains
both an early name override and a dated reset. Province records contain
holding type, slot count, and changes such as a capital move or removal.
The [historical selection notes](reverse-engineering/STARTING-ERAS.md)
document verified examples. Initial runtime object creation and the order
in which the engine applies all inputs remain investigation targets.

## Succession, government, and relationships

Succession determines who inherits when a holder dies. Laws can select one
heir or divide titles. Eligibility and ordering depend on the applicable
succession law and character relationships. For example, primogeniture
favors an eldest eligible line, while gavelkind can divide holdings among
eligible heirs. The next player character and the recipients of individual
titles must be checked separately. Government and realm laws affect the
available actions and obligations. [Paradox CK2 manual, laws and succession](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Start with `common/laws/succession_laws.txt`, `common/governments/`, and
the dated title and character files. A useful test records the ruler, heirs,
title laws, and vassal relationships before a controlled death and checks
each title afterward. Track tie-breaking and update order as open until
observed or traced. Conclave's council powers are an expansion case and
belong in a separate fixture. [Paradox Conclave overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/add-ons/crusader-kings-ii-conclave)

## Economy, holdings, and levies

Holdings are sources of income and troops. Directly controlled holdings
and vassal contributions are separate sources. Buildings change a
holding's capabilities, with requirements and costs that depend on the
building and local conditions. The simulation must combine the installed
definitions with character, law, modifier, and holding state. The exact
tax and levy formulas, rounding, and update timing require a pinned
reference scenario. [Paradox CK2 manual, province and military interfaces](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Inspect `common/buildings/`, `common/laws/`, and the current province and
holder records. Record a monthly income change, one construction from
start to completion, and levies before and after a single controlled
input changes. OpenCK2's prototype income and upgrades are temporary
simulation rules.

## Diplomacy, war, religion, and events

Diplomacy acts on characters and their relationships. Marriages, claims,
opinion, factions, and religious or legal conditions affect which actions
are available. A war's casus belli defines why it can start and what
outcomes can change. Armies move, fight, and besiege holdings. The peace
outcome depends on the war's rules, participants, and result. Religion and
culture also feed into scripted conditions and historical character
identity. [Paradox CK2 manual, diplomacy and warfare](https://cdn.akamai.steamstatic.com/steam/apps/203770/manuals/CKII_MANUEL-ONLINE_FR_HD.pdf?t=1472131858)

Read `common/cb_types/`, `common/combat_tactics/`,
`common/religions/`, `common/objectives/`, and `common/traits/`. A claim
war is a bounded test when claimant, target, casus belli, and peace
outcome are fixed. Extra casus belli can be expansion features. [Paradox Jade Dragon overview](https://www.paradoxinteractive.com/games/crusader-kings-ii/add-ons/crusader-kings-ii-jade-dragon)

The installed `events/`, `decisions/`, `common/on_actions/`,
`common/scripted_triggers/`, and `common/scripted_effects/` describe
scripted behavior. For a reconstruction fixture, select one trigger and
one effect, then capture the actor, scope, eligibility, and state change.
The order of dispatch, random draws, and AI decisions must be measured
or traced for the pinned build.

## Time, rules, saves, and verification

The player can pause the campaign and change its speed while the calendar
advances. The project needs separate evidence for the simulation's day
boundary, scheduled updates, and rule effects. `common/game_rules/`
describes rule options. A menu label or default value establishes the
available choice. To verify its effect, compare the same reference
scenario with a single rule changed.

For save work, begin with an isolated, plain-text non-Ironman fixture.
Compare the restored ruler, titles, holdings, date, and treasury, then
observe the next transition. Record DLC, mods, checksum, and save variant.
The original game's Continue filename handoff has been traced. World
serialization and interoperability are open. [Launcher evidence](reverse-engineering/LAUNCHER-DECOMP.md)

Use the [roadmap](../ROADMAP.md) and
[REA knowledge base](reverse-engineering/README.md) to turn these
mechanics into reproducible comparisons. The original manual is a
player-facing reference from an earlier release. Use the installed
3.3.5.1 data and controlled runs for patch-specific details.
