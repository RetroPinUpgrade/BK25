/**************************************************************************
    BK23 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "RPU_Config.h"
#include "RPU.h"
#include "DropTargets.h"
#include "BK.h"
#include "OperatorMenus.h"
#include "AudioHandler.h"
#include "DisplayHandler.h"
#include "LampAnimations.h"
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
#include "ALB-Communication.h"
#endif
#include <EEPROM.h>

// todo:
//    King's challenge ending announcement
//    Coop mode
//    HORSE mode
//    * last chance should update lock status (not penalize)
//    
//    

#define USE_SCORE_OVERRIDES
#define BK_MAJOR_VERSION  2025
#define BK_MINOR_VERSION  1
#define DEBUG_MESSAGES  1

/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean InOperatorMenu = false;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110
#define MACHINE_STATE_DIAGNOSTICS     120

#define GAME_MODE_NONE                              0
#define GAME_MODE_SKILL_SHOT                        1
#define GAME_MODE_UNSTRUCTURED_PLAY                 2
#define GAME_MODE_BALL_IN_SHOOTER_LANE              3
#define GAME_MODE_OFFER_SINGLE_COMBAT               5
#define GAME_MODE_SINGLE_COMBAT_START               6
#define GAME_MODE_SINGLE_COMBAT                     7
#define GAME_MODE_SINGLE_COMBAT_WON                 8
#define GAME_MODE_SINGLE_COMBAT_LOST                9
#define GAME_MODE_OFFER_DOUBLE_COMBAT               10
#define GAME_MODE_DOUBLE_COMBAT_START               11
#define GAME_MODE_DOUBLE_COMBAT                     12
#define GAME_MODE_DOUBLE_COMBAT_OVER                13
#define GAME_MODE_DOUBLE_COMBAT_FIRST_WIN           14
#define GAME_MODE_DOUBLE_COMBAT_LOST                15
#define GAME_MODE_DOUBLE_COMBAT_LEVEL_INCREASED     16
#define GAME_MODE_DOUBLE_COMBAT_LEVEL_SAME          17
#define GAME_MODE_TRIPLE_COMBAT_START               18
#define GAME_MODE_TRIPLE_COMBAT                     19
#define GAME_MODE_TRIPLE_COMBAT_OVER                20
#define GAME_MODE_TRIPLE_COMBAT_FIRST_WIN           21
#define GAME_MODE_TRIPLE_COMBAT_LOST                22
#define GAME_MODE_TRIPLE_COMBAT_LEVEL_INCREASED     23
#define GAME_MODE_TRIPLE_COMBAT_LEVEL_SAME          24
#define GAME_MODE_SEIGE                             30
#define GAME_MODE_KINGS_CHALLENGE_START             31
#define GAME_MODE_WAIT_FOR_BALL_TO_RETURN           60


#define EEPROM_RPOS_INIT_PROOF_UL                 90
// The proof value for BK25 is "BK01"
#define RPOS_INIT_PROOF                           0x424B3031
#define EEPROM_BALL_SAVE_BYTE                     100
#define EEPROM_FREE_PLAY_BYTE                     101
#define EEPROM_SOUND_SELECTOR_BYTE                102
#define EEPROM_SKILL_SHOT_BYTE                    103
#define EEPROM_TILT_WARNING_BYTE                  104
#define EEPROM_AWARD_OVERRIDE_BYTE                105
#define EEPROM_BALLS_OVERRIDE_BYTE                106
#define EEPROM_TOURNAMENT_SCORING_BYTE            107
#define EEPROM_SFX_VOLUME_BYTE                    108
#define EEPROM_MUSIC_VOLUME_BYTE                  109
#define EEPROM_SCROLLING_SCORES_BYTE              110
#define EEPROM_CALLOUTS_VOLUME_BYTE               111
#define EEPROM_GOALS_UNTIL_WIZ_BYTE               112
#define EEPROM_IDLE_MODE_BYTE                     113
#define EEPROM_WIZ_TIME_BYTE                      114
#define EEPROM_SPINNER_ACCELERATOR_BYTE           115
#define EEPROM_COMBOS_GOAL_BYTE                   116
#define EEPROM_ALLOW_RESET_BYTE                   117
#define EEPROM_CRB_HOLD_TIME                      118
#define EEPROM_DT_HURRYUP_TIME_BYTE               119
#define EEPROM_SINGLE_COMBAT_NUM_SECONDS_BYTE     120
#define EEPROM_DOUBLE_COMBAT_NUM_SECONDS_BYTE     121
#define EEPROM_TRIPLE_COMBAT_NUM_SECONDS_BYTE     122
#define EEPROM_SINGLE_COMBAT_BALL_SAVE_BYTE       123
#define EEPROM_COMBOS_UNTIL_OMNIA_BYTE            124
#define EEPROM_MAX_EXTRA_BALLS_BYTE               125
#define EEPROM_SPINS_UNTIL_GOAL_BYTE              126
#define EEPROM_REQUIRE_PORTCULLIS_BYTE            127
#define EEPROM_TIME_FOR_KCS_BYTE                  128
#define EEPROM_GRACE_TIME_FOR_COMBOS_BYTE         129
#define EEPROM_MAGNA_SAVE_MAX_BYTE                130
#define EEPROM_GI_IN_ORIGINAL_BYTE                131
#define EEPROM_GAME_RULES_SELECTION               137
#define EEPROM_MATCH_FEATURE_BYTE                 138
#define EEPROM_EXTRA_BALL_SCORE_UL                140
#define EEPROM_SPECIAL_SCORE_UL                   144


#define SOUND_EFFECT_NONE                     0
#define SOUND_EFFECT_OUTLANE_UNLIT            4
#define SOUND_EFFECT_DROP_TARGET_COMPLETE_1   7
#define SOUND_EFFECT_DROP_TARGET_COMPLETE_2   8
#define SOUND_EFFECT_DROP_TARGET_COMPLETE_3   9
#define SOUND_EFFECT_HORSE_NEIGHING           10
#define SOUND_EFFECT_HORSE_CHUFFING           11
#define SOUND_EFFECT_FANFARE_1                12
#define SOUND_EFFECT_FANFARE_2                13
#define SOUND_EFFECT_FANFARE_3                14
#define SOUND_EFFECT_SWORD_1                  15
#define SOUND_EFFECT_SWORD_2                  16
#define SOUND_EFFECT_SWORD_3                  17
#define SOUND_EFFECT_SWORD_4                  18
#define SOUND_EFFECT_SWORD_5                  19
#define SOUND_EFFECT_SWORD_6                  20
#define SOUND_EFFECT_SWORD_7                  21
#define SOUND_EFFECT_SPINNER_LIT_2            25
#define SOUND_EFFECT_GAME_OVER                26
#define SOUND_EFFECT_RISING_WARNING           27
#define SOUND_EFFECT_TILT_WARNING             28
#define SOUND_EFFECT_EJECT_WARNING            29
#define SOUND_EFFECT_MATCH_SPIN               30
#define SOUND_EFFECT_POP_BUMPER               31
#define SOUND_EFFECT_TOM_HIT_LEFT             32
#define SOUND_EFFECT_TOM_HIT_RIGHT            33
#define SOUND_EFFECT_SLING_SHOT               34
#define SOUND_EFFECT_MAGNET                   35
#define SOUND_EFFECT_PORTCULLIS               36
#define SOUND_EFFECT_SPINNER_UNLIT            37
#define SOUND_EFFECT_SWOOSH                   38
#define SOUND_EFFECT_DOOR_SLAM                39
#define SOUND_EFFECT_COUNTDOWN_BONUS_START    40
#define SOUND_EFFECT_COUNTDOWN_BONUS_END      50
#define SOUND_EFFECT_CHURCH_BELL_1            51
#define SOUND_EFFECT_COIN_JINGLE_2            52
#define SOUND_EFFECT_COIN_JINGLE_3            53
#define SOUND_EFFECT_SNARE_FILL_1             54
#define SOUND_EFFECT_SNARE_FILL_2             55
#define SOUND_EFFECT_SNARE_FILL_3             56
#define SOUND_EFFECT_THREE_BELLS              57
#define SOUND_EFFECT_BOOING_1                 58
#define SOUND_EFFECT_BOOING_2                 59
#define SOUND_EFFECT_BOOING_3                 60
#define SOUND_EFFECT_10_SECONDS_LEFT          61
#define SOUND_EFFECT_CROWD_CHEERING           62
#define SOUND_EFFECT_THREE_DINGS              63
#define SOUND_EFFECT_THREE_BLOCKS             64
#define SOUND_EFFECT_BONUS_2X_AWARD           65
#define SOUND_EFFECT_BONUS_3X_AWARD           66
#define SOUND_EFFECT_BONUS_4X_AWARD           67
#define SOUND_EFFECT_BONUS_5X_AWARD           68
#define SOUND_EFFECT_BONUS_6X_AWARD           69
#define SOUND_EFFECT_BONUS_7X_AWARD           70
#define SOUND_EFFECT_BONUS_8X_AWARD           71
#define SOUND_EFFECT_BONUS_9X_AWARD           72
#define SOUND_EFFECT_THREE_ANVILS             73
//#define SOUND_EFFECT_SMALL_EXPLOSION          74
#define SOUND_EFFECT_TILT                     75
#define SOUND_EFFECT_SCORE_TICK               76
#define SOUND_EFFECT_LAUGH                    77
#define SOUND_EFFECT_LEVITATE                 78
#define SOUND_EFFECT_SINGLE_ANVIL             79  
#define SOUND_EFFECT_CHURCH_BELL_2            80
#define SOUND_EFFECT_CHURCH_BELL_3            81
#define SOUND_EFFECT_CHURCH_BELL_4            82

#define SOUND_EFFECT_DROP_TARGET_HIT_1        85
#define SOUND_EFFECT_DROP_TARGET_HIT_2        86
#define SOUND_EFFECT_DROP_TARGET_HIT_3        87
#define SOUND_EFFECT_DROP_TARGET_HIT_4        88
#define SOUND_EFFECT_DROP_TARGET_HIT_5        89
#define SOUND_EFFECT_DROP_TARGET_HIT_6        90
#define SOUND_EFFECT_DROP_TARGET_HIT_7        91
#define SOUND_EFFECT_DROP_TARGET_HIT_8        92
#define SOUND_EFFECT_DROP_TARGET_HIT_9        93
#define SOUND_EFFECT_DROP_TARGET_HIT_10       94

#define SOUND_EFFECT_COIN_DROP_1              100
#define SOUND_EFFECT_COIN_DROP_2              101
#define SOUND_EFFECT_COIN_DROP_3              102
#define SOUND_EFFECT_MACHINE_START_1          120
#define SOUND_EFFECT_MACHINE_START_2          120

#define SOUND_EFFECT_SELF_TEST_MODE_START               132
#define SOUND_EFFECT_SELF_TEST_CPC_START                180
#define SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START      190
#define SOUND_EFFECT_SELF_TEST_CRB_OPTIONS_START        210

#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START                 300
#define SOUND_EFFECT_VP_BALL_MISSING                              300
#define SOUND_EFFECT_VP_PLAYER_ONE_UP                             301
#define SOUND_EFFECT_VP_PLAYER_TWO_UP                             302
#define SOUND_EFFECT_VP_PLAYER_THREE_UP                           303
#define SOUND_EFFECT_VP_PLAYER_FOUR_UP                            304
#define SOUND_EFFECT_VP_TIMERS_FROZEN                             307

#define SOUND_EFFECT_VP_ADD_PLAYER_1                              308
#define SOUND_EFFECT_VP_ADD_PLAYER_2                              (SOUND_EFFECT_VP_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3                              (SOUND_EFFECT_VP_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4                              (SOUND_EFFECT_VP_ADD_PLAYER_1+3)

#define SOUND_EFFECT_VP_SHOOT_AGAIN                               312
#define SOUND_EFFECT_VP_BALL_LOCKED                               313
#define SOUND_EFFECT_VP_EXTRA_BALL                                314
#define SOUND_EFFECT_VP_JACKPOT_READY                             315
#define SOUND_EFFECT_VP_LEVEL_1                                   316
#define SOUND_EFFECT_VP_LEVEL_2                                   317
#define SOUND_EFFECT_VP_LEVEL_3                                   318
#define SOUND_EFFECT_VP_LEVEL_4                                   319
#define SOUND_EFFECT_VP_LEVEL_5                                   320
#define SOUND_EFFECT_VP_LEVEL_MAX                                 321
//#define SOUND_EFFECT_VP_BONUS_X_INCREASED                         322
#define SOUND_EFFECT_VP_DOUBLE_COMBAT_FIRST_VICTORY               323
#define SOUND_EFFECT_VP_DOUBLE_COMBAT_LOST                        324
#define SOUND_EFFECT_VP_DOUBLE_COMBAT_LEVEL_INCREASED             325
#define SOUND_EFFECT_VP_DOUBLE_COMBAT_LEVEL_SAME                  326
#define SOUND_EFFECT_VP_TRIPLE_COMBAT_FIRST_VICTORY               327
#define SOUND_EFFECT_VP_TRIPLE_COMBAT_LOST                        328
#define SOUND_EFFECT_VP_TRIPLE_COMBAT_LEVEL_INCREASED             329
#define SOUND_EFFECT_VP_TRIPLE_COMBAT_LEVEL_SAME                  330
#define SOUND_EFFECT_VP_RELIC_1                                   331
#define SOUND_EFFECT_VP_RELIC_2                                   332
#define SOUND_EFFECT_VP_RELIC_3                                   333
#define SOUND_EFFECT_VP_RELIC_4                                   334
#define SOUND_EFFECT_VP_RELIC_5                                   335
#define SOUND_EFFECT_VP_RELIC_6                                   336
#define SOUND_EFFECT_VP_RELIC_7                                   337
#define SOUND_EFFECT_VP_RELIC_8                                   338
#define SOUND_EFFECT_VP_RELIC_9                                   339
#define SOUND_EFFECT_VP_PRESS_FOR_SINGLE                          340
#define SOUND_EFFECT_VP_PRESS_FOR_DOUBLE                          341
#define SOUND_EFFECT_VP_SINGLE_COMBAT                             342
#define SOUND_EFFECT_VP_DOUBLE_COMBAT                             343
#define SOUND_EFFECT_VP_TRIPLE_COMBAT                             344
#define SOUND_EFFECT_VP_SINGLE_HINT_PART_3                        345
#define SOUND_EFFECT_VP_DOUBLE_HINT_PART_1                        346
#define SOUND_EFFECT_VP_JACKPOT                                   347
#define SOUND_EFFECT_VP_DOUBLE_JACKPOT                            348
#define SOUND_EFFECT_VP_TRIPLE_JACKPOT                            349
#define SOUND_EFFECT_VP_SUPER_JACKPOT                             350
#define SOUND_EFFECT_VP_MEGA_JACKPOT                              351
#define SOUND_EFFECT_VP_SINGLE_COMBAT_INSTRUCTIONS                352
#define SOUND_EFFECT_VP_DEATH_BLOW_INCREASED                      353
#define SOUND_EFFECT_VP_30_SECONDS                                354
#define SOUND_EFFECT_VP_45_SECONDS                                355
#define SOUND_EFFECT_VP_60_SECONDS                                356
#define SOUND_EFFECT_VP_75_SECONDS                                357
#define SOUND_EFFECT_VP_90_SECONDS                                358
#define SOUND_EFFECT_VP_120_SECONDS                               359
#define SOUND_EFFECT_VP_SINGLE_LOST                               360
#define SOUND_EFFECT_VP_DOUBLE_LOST                               361
#define SOUND_EFFECT_VP_TRIPLE_LOST                               362
#define SOUND_EFFECT_VP_RELIC_ALL                                 363
//
#define SOUND_EFFECT_VP_SKILL_SHOT                                365
#define SOUND_EFFECT_VP_SUPER_SKILL_SHOT_1                        366
#define SOUND_EFFECT_VP_SUPER_SKILL_SHOT_2                        367
#define SOUND_EFFECT_VP_RETURN_TO_FIGHT                           368
#define SOUND_EFFECT_VP_SAUCER_FOR_DEATHBLOW                      369
#define SOUND_EFFECT_VP_PRESS_FOR_SINGLE_PART_2                   370
#define SOUND_EFFECT_VP_PRESS_FOR_SINGLE_PART_3                   371
#define SOUND_EFFECT_VP_SINGLE_HINT_PART_1                        372
#define SOUND_EFFECT_VP_SINGLE_HINT_PART_2                        373
//
#define SOUND_EFFECT_VP_SINGLE_TEN_SECONDS_TO_HIT_SAUCER          375
#define SOUND_EFFECT_VP_SINGLE_OPPONENT_RALLIES                   376
#define SOUND_EFFECT_VP_SINGLE_COMBAT_PART_1_COMPLETE             377
#define SOUND_EFFECT_VP_SINGLE_COMBAT_PART_2_COMPLETE             378
#define SOUND_EFFECT_VP_SINGLE_COMBAT_PART_3_COMPLETE             379
#define SOUND_EFFECT_VP_PRESS_FOR_DOUBLE_PART_2                   380
#define SOUND_EFFECT_VP_PRESS_FOR_DOUBLE_PART_3                   381
#define SOUND_EFFECT_VP_DOUBLE_HINT_2_OR_3                        382
#define SOUND_EFFECT_VP_BONUS_X_COLLECT_INSTRUCTIONS              383
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_AVAILABLE                 384
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_1                         385
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_2                         386
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_3                         387
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_4                         388
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_JOUST                     389
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_PERFECTION                390
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_LEVITATE                  391
#define SOUND_EFFECT_VP_KINGS_CHALLENGE_MELEE                     392
#define SOUND_EFFECT_VP_RETURN_TO_1X                              393
#define SOUND_EFFECT_VP_PLAYFIELD_2X                              394
#define SOUND_EFFECT_VP_PLAYFIELD_3X                              395
#define SOUND_EFFECT_VP_PLAYFIELD_4X                              396
#define SOUND_EFFECT_VP_PLAYFIELD_5X                              397
#define SOUND_EFFECT_VP_SINGLE_HINT_0                             398
//#define SOUND_EFFECT_VP_DOUBLE_HINT_0                             399
#define SOUND_EFFECT_VP_TRIPLE_HINT_0                             400
#define SOUND_EFFECT_VP_TRIPLE_HINT_1                             401
#define SOUND_EFFECT_VP_TRIPLE_HINT_2                             402
#define SOUND_EFFECT_VP_TRIPLE_HINT_3                             403
#define SOUND_EFFECT_VP_TRIPLE_JACKPOTS_READY                     404
#define SOUND_EFFECT_VP_JACKPOT_ALT_0                             405
#define SOUND_EFFECT_VP_JACKPOT_ALT_1                             406
#define SOUND_EFFECT_VP_JACKPOT_ALT_2                             407
#define SOUND_EFFECT_VP_JACKPOT_ALT_3                             408
#define SOUND_EFFECT_VP_JACKPOT_ALT_4                             409
#define SOUND_EFFECT_VP_TRIPLE_SUPER_READY                        410
#define SOUND_EFFECT_VP_KC_QUALIFIED                              411
#define SOUND_EFFECT_VP_SKILL_SHOT_MISSED_1                       412
#define SOUND_EFFECT_VP_SKILL_SHOT_MISSED_2                       413
#define SOUND_EFFECT_VP_SKILL_SHOT_MISSED_3                       414
#define SOUND_EFFECT_VP_SKILL_SHOT_MISSED_4                       415
#define SOUND_EFFECT_VP_SKILL_SHOT_MISSED_5                       416
#define SOUND_EFFECT_VP_GOOD_SHOT_1                               417
#define SOUND_EFFECT_VP_GOOD_SHOT_2                               418
#define SOUND_EFFECT_VP_GOOD_SHOT_3                               419
#define SOUND_EFFECT_VP_FIGHT_AGAIN                               420
#define SOUND_EFFECT_VP_SHOOT_AGAIN                               421
#define SOUND_EFFECT_VP_CURRENT_JACKPOT                           422
#define SOUND_EFFECT_VP_GAME_RULES_EASY                           423
#define SOUND_EFFECT_VP_GAME_RULES_MEDIUM                         424
#define SOUND_EFFECT_VP_GAME_RULES_HARD                           425




#define SOUND_EFFECT_RALLY_SONG_1         500
#define SOUND_EFFECT_RALLY_SONG_2         501
#define SOUND_EFFECT_RALLY_SONG_3         502
#define SOUND_EFFECT_RALLY_PLUNGE         520
#define SOUND_EFFECT_BACKGROUND_SONG_1    525
#define SOUND_EFFECT_BACKGROUND_SONG_2    526
#define SOUND_EFFECT_BACKGROUND_SONG_3    527
#define NUM_BACKGROUND_SONGS              11
#define SOUND_EFFECT_BATTLE_SONG_1        575
#define NUM_BATTLE_SONGS                  3


#define SOUND_EFFECT_DIAG_START                   1900
#define SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON     1900
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON      1901
#define SOUND_EFFECT_DIAG_SELECTOR_SWITCH_OFF     1902
#define SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE  1903
#define SOUND_EFFECT_DIAG_STARTING_NEW_CODE       1904
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_DETECTED   1905
#define SOUND_EFFECT_DIAG_ORIGINAL_CPU_RUNNING    1906
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U10         1907
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_U11         1908
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_1           1909
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_2           1910
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_3           1911
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_4           1912
#define SOUND_EFFECT_DIAG_PROBLEM_PIA_5           1913
#define SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS    1914


#define GAME_RULES_EASY         1
#define GAME_RULES_MEDIUM       2
#define GAME_RULES_HARD         3
#define GAME_RULES_PROGRESSIVE  4
#define GAME_RULES_CUSTOM       5


#define MAX_DISPLAY_BONUS     109

#define TILT_WARNING_DEBOUNCE_TIME      1000


/*********************************************************************

    Machine state and options

*********************************************************************/
byte Credits = 0;
byte SoundSelector = 3;
byte BallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte ScoreAwardReplay = 0;
byte MusicVolume = 6;
byte SoundEffectsVolume = 8;
byte CalloutsVolume = 10;
byte ChuteCoinsInProgress[3];
byte TotalBallsLoaded = 3;
byte TimeRequiredToResetGame = 2;
byte NumberOfBallsInPlay = 0;
byte NumberOfBallsLocked = 0;
byte GameRulesSelection = GAME_RULES_MEDIUM;
boolean FreePlayMode = false;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ScrollingScores = true;
boolean GIInOriginal = true;
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long CreditResetPressStarted = 0;
unsigned long SaucerEjectTime = 0;
unsigned long LastTimeBallServed = 0;
unsigned long OperatorSwitchPressStarted = 0;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
unsigned long ALBWatchdogResetLastSent;
#endif

#define NUM_CPC_PAIRS 9
boolean CPCSelectionsHaveBeenRead = false;
byte CPCPairs[NUM_CPC_PAIRS][2] = {
  {1, 5},
  {1, 4},
  {1, 3},
  {1, 2},
  {1, 1},
  {2, 3},
  {2, 1},
  {3, 1},
  {4, 1}
};
byte CPCSelection[3];

AudioHandler Audio;
OperatorMenus Menus;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
AccessoryLampBoard TopperALB;
#endif

/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte WaitingForBallGameMode = GAME_MODE_NONE;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte CurrentAchievements[4];
byte ExtraBallsCollected = 0;

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean SpecialCollected = false;
boolean TimersPaused = false;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long PlayfieldMultiplier;
unsigned long LastTimeThroughLoop;
unsigned long LastSwitchHitTime;
unsigned long BallSaveEndTime;

#define BALL_SAVE_GRACE_PERIOD  2000

#define GI_OVERRIDE_NUMBER_OF_HOLDS   10
boolean GIReturnState;
unsigned long GIOverrideOffTime[GI_OVERRIDE_NUMBER_OF_HOLDS];
unsigned short GIOverrideHoldTime[GI_OVERRIDE_NUMBER_OF_HOLDS];

/*********************************************************************

    Game Specific State Variables

*********************************************************************/
byte SpinsTowardsNextGoal[4];
byte SpinnerGoal[4];
byte BaseSpinsUntilSpinnerGoal = 25;
byte IdleMode;
byte TimeToResetDrops = 8;
byte KingsChallengeBaseTime = 20;
byte NumDropTargetHits[4][4];
byte NumDropTargetClears[4][4];
byte LastChanceStatus[4];
byte DropTargetLevel[4];
byte SkillShotsMade[4];
byte SkillShotTarget;
byte SpinnerStatus;
byte MaxExtraBallsPerBall = 2;
byte BaseComboTime = 25;
byte MagnaSaveMaxSeconds = 5;
//byte NumberOfComboDefinitions = 0;
byte SingleCombatNumSeconds = 60;
byte DoubleCombatNumSeconds = 30;
byte TripleCombatNumSeconds = 30;
byte SingleCombatLevelCompleted[4];
byte DoubleCombatLevelCompleted[4];
byte TripleCombatLevelCompleted[4];
byte BallSaveOnCombatModes = 20;
byte NumBonusXCollectReminders;
byte ComboStep;
byte LastComboSwitch;
byte CombosUntilOmnia = 16;
byte TripleCombatJackpotsAvailable;
#define TRIPLE_COMBAT_ALL_JACKPOTS            0x3F
#define TRIPLE_COMBAT_SPINNER_JACKPOT         0x01
#define TRIPLE_COMBAT_MIDDLE_RAMP_JACKPOT     0x02
#define TRIPLE_COMBAT_LOOP_JACKPOT            0x04
#define TRIPLE_COMBAT_SAUCER_JACKPOT          0x08
#define TRIPLE_COMBAT_LOCK_JACKPOT            0x10
#define TRIPLE_COMBAT_UPPER_RAMP_JACKPOT      0x20
#define TRIPLE_COMBAT_SUPER_JACKPOT           0x40

byte CombatBankFlags;
#define DROP_BANK_UL_FLAG   0x01
#define DROP_BANK_UR_FLAG   0x02
#define DROP_BANK_LL_FLAG   0x04
#define DROP_BANK_LR_FLAG   0x08

#define LAST_CHANCE_LEFT_QUALIFIED    0x01
#define LAST_CHANCE_RIGHT_QUALIFIED   0x02

#define COMBO_RIGHT_INLANE_SPINNER                    0x0001
#define COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP          0x0002
#define COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP_LOCK     0x0004
#define COMBO_RIGHT_INLANE_LOOP                       0x0008
#define COMBO_LEFT_INLANE_RAMP                        0x0010
#define COMBO_LEFT_INLANE_SAUCER                      0x0020
#define COMBO_SAUCER_RAMP                             0x0040
#define COMBO_SAUCER_SAUCER                           0x0080
#define COMBO_LOOP_LEFT_INLANE                        0x0100
#define COMBO_LEFT_RIGHT_PASS                         0x0200
#define COMBO_RIGHT_LEFT_PASS                         0x0400
#define COMBO_LEFT_INLANE_RAMP_LOCK                   0x0800
#define COMBO_ALL                                     0x8000


/*
#define NUM_BALL_SEARCH_SOLENOIDS   3
byte BallSearchSolenoidToTry;
byte BallSearchSols[NUM_BALL_SEARCH_SOLENOIDS] = {SOL_POP_BUMPER, SOL_RIGHT_SLING, SOL_LEFT_SLING};
#define BALL_SEARCH_POP_INDEX 0
#define BALL_SEARCH_LEFT_SLING_INDEX  1
#define BALL_SEARCH_RIGHT_SLING_INDEX 2
*/
// Machine locks is a curated variable that keeps debounced
// knowledge of how many balls are trapped in different mechanisms.
// It's initialized the first time that attract mode is run,
// and then it's maintained by "UpdateLockStatus", which only
// registers change when a state is changed for a certain
// amount of time.
byte MachineLocks;
byte PlayerLockStatus[4];

#define LOCK_1_ENGAGED        0x10
#define LOCK_2_ENGAGED        0x20
#define LOCK_3_ENGAGED        0x40
#define LOCKS_ENGAGED_MASK    0x70
#define LOCK_1_AVAILABLE      0x01
#define LOCK_2_AVAILABLE      0x02
#define LOCK_3_AVAILABLE      0x04
#define LOCKS_AVAILABLE_MASK  0x07

byte ExtraBallsOrSpecialAvailable[4];
#define EBS_LOWER_EXTRA_BALL_AVAILABLE      0x01
#define EBS_LOWER_SPECIAL_AVAILABLE         0x02
#define EBS_UPPER_EXTRA_BALL_AVAILABLE      0x04

byte KingsChallengeKick; // 1 = top lock, 2 = saucer
byte KingsChallengeStatus[4];
byte KingsChallengePerfectionBank;
byte KingsChallengeRunning;
#define KINGS_CHALLENGE_1_QUALIFIED         0x01
#define KINGS_CHALLENGE_2_QUALIFIED         0x02
#define KINGS_CHALLENGE_3_QUALIFIED         0x04
#define KINGS_CHALLENGE_4_QUALIFIED         0x08
#define KINGS_CHALLENGE_AVAILABLE           0x0F
#define KINGS_CHALLENGE_DROPS               0x07
#define KINGS_CHALLENGE_1_COMPLETE          0x10
#define KINGS_CHALLENGE_2_COMPLETE          0x20
#define KINGS_CHALLENGE_3_COMPLETE          0x40
#define KINGS_CHALLENGE_4_COMPLETE          0x80
#define KINGS_CHALLENGE_JOUST               KINGS_CHALLENGE_1_QUALIFIED
#define KINGS_CHALLENGE_PERFECTION          KINGS_CHALLENGE_2_QUALIFIED
#define KINGS_CHALLENGE_LEVITATE            KINGS_CHALLENGE_3_QUALIFIED
#define KINGS_CHALLENGE_MELEE               KINGS_CHALLENGE_4_QUALIFIED


boolean IdleModeEnabled = true;
boolean OutlaneSpecialLit[4];
boolean DropTargetHurryLamp[4];
boolean CombatJackpotReady;
boolean CombatSuperJackpotReady;
boolean LoopLitToQualifyLock;
boolean MagnaSaveAvailable;
boolean MagnaSoundOn;
boolean RequirePortcullisForLocks = true;
boolean LockManagementInProgress;
boolean BonusXCollectAvailable;
boolean DoubleTimers;
boolean LastChanceAvailable;
boolean KCBeginPlayed;
boolean ReturnToFightAlreadyPlayed;


boolean SpinnerLitFromCombo = false;
boolean RightRampLitFromCombo = false;
boolean SaucerLitFromCombo = false;
boolean LockLitFromCombo = false;
boolean LoopLitFromCombo = false;
boolean LeftInlaneLitFromCombo = false;
boolean RightInlaneLitFromCombo = false;
boolean UpperLeftRolloverLitFromCombo = false;

unsigned short RightRampValue[4];

unsigned long LastSaucerHitTime = 0;
unsigned long BonusXCollectAvailableStart;
unsigned long BonusXCollectReminder;
unsigned long BonusXAnimationStart;
unsigned long LastSpinnerHit;
unsigned long LastPopBumperHit;
unsigned long SpinnerLitUntil;
unsigned long KingsChallengeEndTime;
unsigned long KingsChallengeBonus;
unsigned long KingsChallengeBonusChangedTime;
unsigned long LevitateMagnetOnTime;
unsigned long LevitateMagnetOffTime;
unsigned long LastTimeLeftMagnetOn;
unsigned long LastTimeRightMagnetOn;

unsigned long PlayfieldMultiplierExpiration;
//unsigned long BallSearchNextSolenoidTime;
//unsigned long BallSearchSolenoidFireTime[NUM_BALL_SEARCH_SOLENOIDS];
unsigned long TicksCountedTowardsStatus;
unsigned long BallRampKicked;
unsigned long DropTargetResetTime[4];
unsigned long DropTargetHurryTime[4];
unsigned long MagnaStatusLeft[4];
unsigned long MagnaStatusRight[4];
unsigned long LockKickTime[3];
unsigned long PlayLockKickWarningTime;
unsigned long SaucerKickTime;
unsigned long PlaySaucerKickWarningTime;
unsigned long ExtraBallAwardStartTime;
unsigned long LastLoopHitTime;

// Combo tracking variables
unsigned long LastLeftInlane;
unsigned long LastRightInlane;
unsigned long LastLeftOutlane;
unsigned long LastRightOutlane;
unsigned long CombosAchieved[4];

unsigned long LeftComboLastHitTime;
unsigned long RightComboLastHitTime;

unsigned long JackpotIncreasedTime;
unsigned long CombatJackpot[4];
unsigned long JackpotBeforeCombat;
#define COMBAT_JACKPOT_BASE_1     100000
#define COMBAT_JACKPOT_BASE_2     125000
#define COMBAT_JACKPOT_BASE_3     25000
#define COMBAT_JACKPOT_STEP       25000
unsigned long BonusAnimationStart;

DropTargetBank DropTargetsUL(3, 1, DROP_TARGET_TYPE_WLLMS_2, 50);
DropTargetBank DropTargetsUR(3, 1, DROP_TARGET_TYPE_WLLMS_2, 50);
DropTargetBank DropTargetsLL(3, 1, DROP_TARGET_TYPE_WLLMS_2, 50);
DropTargetBank DropTargetsLR(3, 1, DROP_TARGET_TYPE_WLLMS_2, 50);

#define IDLE_MODE_NONE                  0
#define IDLE_MODE_BALL_SEARCH           9

#define SKILL_SHOT_AWARD          25000
#define SUPER_SKILL_SHOT_AWARD    50000

#define POP_BUMPER_DEBOUNCE_TIME    200

/******************************************************

   Adjustments Serialization

*/

void SetAllParameterDefaults() {
  // Basic
  FreePlayMode = false;
  BallSaveNumSeconds = 15;
  MaxTiltWarnings = 2;
  MusicVolume = 10;
  SoundEffectsVolume = 10;
  CalloutsVolume = 10;
  BallsPerGame = 3;
  TournamentScoring = false;
  ExtraBallValue = 50000;
  SpecialValue = 75000;
  TimeRequiredToResetGame = 2;
  AwardScores[0] = 1000000;
  AwardScores[1] = 3000000;
  AwardScores[2] = 5000000;
  ScoreAwardReplay = 0x07;
  ScrollingScores = true;
  HighScore = 10000;
  Credits = 4;
  CPCSelection[0] = 4;
  CPCSelection[1] = 1;
  CPCSelection[2] = 4;
  MatchFeature = true;
  GIInOriginal = true;

  // EASY / MEDIUM / HARD rules
  GameRulesSelection = GAME_RULES_MEDIUM;

  // Rules
  TimeToResetDrops = 8;
  SingleCombatNumSeconds = 60;
  BallSaveOnCombatModes = 20;
  DoubleCombatNumSeconds = 30;
  TripleCombatNumSeconds = 30;
  CombosUntilOmnia = 16;
  MaxExtraBallsPerBall = 2;
  RequirePortcullisForLocks = true;
  KingsChallengeBaseTime = 20;
  BaseComboTime = 25;
  MagnaSaveMaxSeconds = 5;
  BaseSpinsUntilSpinnerGoal = 25;

}


boolean LoadRuleDefaults(byte ruleLevel) {
  // This function just puts the rules in RAM.
  // It's up to the caller to ensure that they're 
  // written to EEPROM when desired (with WriteParameters)
  if (ruleLevel==GAME_RULES_EASY) {
    BallSaveNumSeconds = 20;
    TimeToResetDrops = 11;
    SingleCombatNumSeconds = 120;
    BallSaveOnCombatModes = 40;
    DoubleCombatNumSeconds = 60;
    TripleCombatNumSeconds = 20;
    CombosUntilOmnia = 14;
    MaxExtraBallsPerBall = 3;
    RequirePortcullisForLocks = false;
    KingsChallengeBaseTime = 30;
    BaseComboTime = 35;
    MagnaSaveMaxSeconds = 8;
    BaseSpinsUntilSpinnerGoal = 20;
  } else if (ruleLevel==GAME_RULES_MEDIUM) {
    BallSaveNumSeconds = 10;
    TimeToResetDrops = 8;
    SingleCombatNumSeconds = 60;
    BallSaveOnCombatModes = 20;
    DoubleCombatNumSeconds = 30;
    TripleCombatNumSeconds = 30;
    CombosUntilOmnia = 16;
    MaxExtraBallsPerBall = 2;
    RequirePortcullisForLocks = true;
    KingsChallengeBaseTime = 20;
    BaseComboTime = 25;
    MagnaSaveMaxSeconds = 5;
    BaseSpinsUntilSpinnerGoal = 25;
  } else if (ruleLevel==GAME_RULES_HARD) {
    BallSaveNumSeconds = 0;
    TimeToResetDrops = 4;
    SingleCombatNumSeconds = 30;
    BallSaveOnCombatModes = 0;
    DoubleCombatNumSeconds = 0;
    TripleCombatNumSeconds = 0;
    CombosUntilOmnia = 21;
    MaxExtraBallsPerBall = 1;
    RequirePortcullisForLocks = true;
    KingsChallengeBaseTime = 15;
    BaseComboTime = 20;
    MagnaSaveMaxSeconds = 2;
    BaseSpinsUntilSpinnerGoal = 35;
  } else {
    return false;
  }

  return true;
}



void WriteParameters(boolean onlyWriteRulesParameters = true) {
  if (!onlyWriteRulesParameters) {
    RPU_WriteByteToEEProm(EEPROM_FREE_PLAY_BYTE, FreePlayMode);
    // * Ball save is considered a rule in this game, so it's below
    RPU_WriteByteToEEProm(EEPROM_TILT_WARNING_BYTE, MaxTiltWarnings);  
    RPU_WriteByteToEEProm(EEPROM_MUSIC_VOLUME_BYTE, MusicVolume);
    RPU_WriteByteToEEProm(EEPROM_SFX_VOLUME_BYTE, SoundEffectsVolume);
    RPU_WriteByteToEEProm(EEPROM_CALLOUTS_VOLUME_BYTE, CalloutsVolume);
    RPU_WriteByteToEEProm(EEPROM_BALLS_OVERRIDE_BYTE, BallsPerGame);
    RPU_WriteByteToEEProm(EEPROM_TOURNAMENT_SCORING_BYTE, TournamentScoring);
    RPU_WriteULToEEProm(EEPROM_EXTRA_BALL_SCORE_UL, ExtraBallValue);
    RPU_WriteULToEEProm(EEPROM_SPECIAL_SCORE_UL, SpecialValue);
    RPU_WriteByteToEEProm(EEPROM_CRB_HOLD_TIME, TimeRequiredToResetGame);
    RPU_WriteULToEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE, AwardScores[0]);
    RPU_WriteULToEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE, AwardScores[1]);
    RPU_WriteULToEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE, AwardScores[2]);
    RPU_WriteByteToEEProm(EEPROM_AWARD_OVERRIDE_BYTE, ScoreAwardReplay);
    RPU_WriteByteToEEProm(EEPROM_SCROLLING_SCORES_BYTE, ScrollingScores);
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, HighScore);
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, CPCSelection[0]);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, CPCSelection[1]);
    RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, CPCSelection[2]);
    RPU_WriteByteToEEProm(EEPROM_MATCH_FEATURE_BYTE, MatchFeature);
    RPU_WriteByteToEEProm(EEPROM_GI_IN_ORIGINAL_BYTE, GIInOriginal);

    // Set baseline for audits
    RPU_WriteByteToEEProm(RPU_CHUTE_1_COINS_START_BYTE, 0);
    RPU_WriteByteToEEProm(RPU_CHUTE_2_COINS_START_BYTE, 0);
    RPU_WriteByteToEEProm(RPU_CHUTE_3_COINS_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, 0);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, 0);

  }

  RPU_WriteByteToEEProm(EEPROM_BALL_SAVE_BYTE, BallSaveNumSeconds);
 
  RPU_WriteByteToEEProm(EEPROM_DT_HURRYUP_TIME_BYTE, TimeToResetDrops);
  RPU_WriteByteToEEProm(EEPROM_SINGLE_COMBAT_NUM_SECONDS_BYTE, SingleCombatNumSeconds);
  RPU_WriteByteToEEProm(EEPROM_SINGLE_COMBAT_BALL_SAVE_BYTE, BallSaveOnCombatModes);
  RPU_WriteByteToEEProm(EEPROM_DOUBLE_COMBAT_NUM_SECONDS_BYTE, DoubleCombatNumSeconds);
  RPU_WriteByteToEEProm(EEPROM_TRIPLE_COMBAT_NUM_SECONDS_BYTE, TripleCombatNumSeconds);
  RPU_WriteByteToEEProm(EEPROM_COMBOS_UNTIL_OMNIA_BYTE, CombosUntilOmnia);
  RPU_WriteByteToEEProm(EEPROM_MAX_EXTRA_BALLS_BYTE, MaxExtraBallsPerBall);
  RPU_WriteByteToEEProm(EEPROM_REQUIRE_PORTCULLIS_BYTE, RequirePortcullisForLocks);
  RPU_WriteByteToEEProm(EEPROM_TIME_FOR_KCS_BYTE, KingsChallengeBaseTime);
  RPU_WriteByteToEEProm(EEPROM_GRACE_TIME_FOR_COMBOS_BYTE, BaseComboTime);
  RPU_WriteByteToEEProm(EEPROM_MAGNA_SAVE_MAX_BYTE, MagnaSaveMaxSeconds);
  RPU_WriteByteToEEProm(EEPROM_SPINS_UNTIL_GOAL_BYTE, BaseSpinsUntilSpinnerGoal);
   
}


void ReadStoredParameters() {
  for (byte count = 0; count < 3; count++) {
    ChuteCoinsInProgress[count] = 0;
  }

  unsigned long RPUProofValue = RPU_ReadULFromEEProm(EEPROM_RPOS_INIT_PROOF_UL, 0);
  if (RPUProofValue!=RPOS_INIT_PROOF) {
    // Doesn't look like this memory has been initialized
    RPU_WriteULToEEProm(EEPROM_RPOS_INIT_PROOF_UL, RPOS_INIT_PROOF);
    SetAllParameterDefaults();
    WriteParameters(false);
  } else {
    FreePlayMode = ReadSetting(EEPROM_FREE_PLAY_BYTE, false, true);
    BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15, 20);
    MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2, 3);

    MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10, 10);
    SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10, 10);
    CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10, 10);
    Audio.SetMusicVolume(MusicVolume);
    Audio.SetSoundFXVolume(SoundEffectsVolume);
    Audio.SetNotificationsVolume(CalloutsVolume);

    BallsPerGame = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 3, 10);
    TournamentScoring = ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, false, true);
    ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_UL);
    if (ExtraBallValue % 1000 || ExtraBallValue > 1000000) ExtraBallValue = 20000;
    SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_UL);
    if (SpecialValue % 1000 || SpecialValue > 1000000) SpecialValue = 40000;
    TimeRequiredToResetGame = ReadSetting(EEPROM_CRB_HOLD_TIME, 1, 99);
    if (TimeRequiredToResetGame > 3 && TimeRequiredToResetGame != 99) TimeRequiredToResetGame = 1;
    AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
    AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
    AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);
    ScoreAwardReplay = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 0x03, 0x07);
    ScrollingScores = ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, true, true);
    HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
    Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    CPCSelection[0] = ReadSetting(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4, 8);
    CPCSelection[1] = ReadSetting(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4, 8);
    CPCSelection[2] = ReadSetting(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4, 8);
    CPCSelectionsHaveBeenRead = true;
    MatchFeature = ReadSetting(EEPROM_MATCH_FEATURE_BYTE, true, true);
    GIInOriginal = ReadSetting(EEPROM_GI_IN_ORIGINAL_BYTE, true, true);
    
    // Read game rules
    GameRulesSelection = ReadSetting(EEPROM_GAME_RULES_SELECTION, GAME_RULES_MEDIUM, GAME_RULES_CUSTOM);

    // Rules settings
    TimeToResetDrops = ReadSetting(EEPROM_DT_HURRYUP_TIME_BYTE, 8, 12);
    SingleCombatNumSeconds = ReadSetting(EEPROM_SINGLE_COMBAT_NUM_SECONDS_BYTE, 60, 120);
    BallSaveOnCombatModes = ReadSetting(EEPROM_SINGLE_COMBAT_BALL_SAVE_BYTE, 20, 40);
    DoubleCombatNumSeconds = ReadSetting(EEPROM_DOUBLE_COMBAT_NUM_SECONDS_BYTE, 30, 60);
    TripleCombatNumSeconds = ReadSetting(EEPROM_TRIPLE_COMBAT_NUM_SECONDS_BYTE, 10, 20);
    CombosUntilOmnia = ReadSetting(EEPROM_COMBOS_UNTIL_OMNIA_BYTE, 18, 21);
    MaxExtraBallsPerBall = ReadSetting(EEPROM_MAX_EXTRA_BALLS_BYTE, 2, 3);
    RequirePortcullisForLocks = ReadSetting(EEPROM_REQUIRE_PORTCULLIS_BYTE, true, true);
    KingsChallengeBaseTime = ReadSetting(EEPROM_TIME_FOR_KCS_BYTE, 20, 35);
    BaseComboTime = ReadSetting(EEPROM_GRACE_TIME_FOR_COMBOS_BYTE, 25, 40);
    MagnaSaveMaxSeconds = ReadSetting(EEPROM_MAGNA_SAVE_MAX_BYTE, 5, 8);
    BaseSpinsUntilSpinnerGoal = ReadSetting(EEPROM_SPINS_UNTIL_GOAL_BYTE, 25, 40);
       
  }

}


byte GetCPCSelection(byte chuteNumber) {
  if (chuteNumber>2) return 0xFF;

  if (CPCSelectionsHaveBeenRead==false) {
    CPCSelection[0] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE);
    if (CPCSelection[0]>=NUM_CPC_PAIRS) {
      CPCSelection[0] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_1_SELECTION_BYTE, 4);
    }
    CPCSelection[1] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE);  
    if (CPCSelection[1]>=NUM_CPC_PAIRS) {
      CPCSelection[1] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_2_SELECTION_BYTE, 4);
    }
    CPCSelection[2] = RPU_ReadByteFromEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE);  
    if (CPCSelection[2]>=NUM_CPC_PAIRS) {
      CPCSelection[2] = 4;
      RPU_WriteByteToEEProm(RPU_CPC_CHUTE_3_SELECTION_BYTE, 4);
    }
    CPCSelectionsHaveBeenRead = true;
  }
  
  return CPCSelection[chuteNumber];
}


byte GetCPCCoins(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][0];
}


byte GetCPCCredits(byte cpcSelection) {
  if (cpcSelection>=NUM_CPC_PAIRS) return 1;
  return CPCPairs[cpcSelection][1];
}


byte LampConvertDisplayNumberToIndex(byte displayNumber) {
  if (displayNumber>=43 && displayNumber<=46) return OPERATOR_MENU_VALUE_UNUSED;
  if (displayNumber>0 && displayNumber<65) return displayNumber - 1;
  return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
}


unsigned short SolenoidConvertDisplayNumberToIndex(byte displayNumber) {
  switch (displayNumber) {
    case  0: return OPERATOR_MENU_VALUE_UNUSED;
    case  1: return SOL_OUTHOLE;
    case  2: return SOL_LL_DROP_RESET;
    case  3: return SOL_LR_DROP_RESET;
    case  4: return SOL_UL_DROP_RESET;
    case  5: return SOL_UR_DROP_RESET;
    case  6: return SOL_BALL_RAMP_THROWER;
    case  7: return SOL_UPPER_BALL_EJECT;
    case  8: return SOL_SAUCER;
    case  9: return SOL_RIGHT_MAGNA_SAVE;
    case 10: return SOL_LEFT_MAGNA_SAVE;
    case 11: return SOL_GI_RELAY;
    case 12: return OPERATOR_MENU_VALUE_UNUSED;
    case 13: return OPERATOR_MENU_VALUE_UNUSED;
    case 14: return OPERATOR_MENU_VALUE_UNUSED;
    case 15: return SOL_BELL; // Flipper Mute
    case 16: return SOLCONT_COIN_LOCKOUT;
    case 17: return SOL_LEFT_SLING; // unused 0x1000
    case 18: return SOL_RIGHT_SLING; // unused 0x8000
    case 19: return SOL_POP_BUMPER; // Topper
    case 20: return OPERATOR_MENU_VALUE_UNUSED;
    case 21: return OPERATOR_MENU_VALUE_UNUSED;
    case 22: return OPERATOR_MENU_VALUE_UNUSED;
    default: return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
  }
}


byte SolenoidConvertDisplayNumberToTestStrength(byte displayNumber) {
  switch (displayNumber) {
    case  0: return OPERATOR_MENU_VALUE_UNUSED;
    case  1: return 16;
    case  2: return 50;
    case  3: return 50;
    case  4: return 50;
    case  5: return 50;
    case  6: return 16;
    case  7: return 12;
    case  8: return 12;
    case  9: return 5;
    case 10: return 5;
    case 11: return 10;
    case 12: return OPERATOR_MENU_VALUE_UNUSED;
    case 13: return OPERATOR_MENU_VALUE_UNUSED;
    case 14: return OPERATOR_MENU_VALUE_UNUSED;
    case 15: return 10; 
    case 16: return 10;
    case 17: return 10;
    case 18: return 10;
    case 19: return 10; 
    case 20: return OPERATOR_MENU_VALUE_UNUSED;
    case 21: return OPERATOR_MENU_VALUE_UNUSED;
    case 22: return OPERATOR_MENU_VALUE_UNUSED;
    default: return OPERATOR_MENU_VALUE_OUT_OF_RANGE;
  }
}


void MoveBallFromOutholeToRamp(boolean sawSwitch = false) {
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE) || sawSwitch) {
    if (CurrentTime == 0 || CurrentTime > (BallRampKicked + 1000)) {
      RPU_PushToSolenoidStack(SOL_OUTHOLE, 16, true);
      if (CurrentTime) BallRampKicked = CurrentTime;
      else BallRampKicked = millis();
    }
  }

}


void QueueDIAGNotification(unsigned short notificationNum) {
  // This is optional, but the machine can play an audio message at boot
  // time to indicate any errors and whether it's going to boot to original
  // or new code.
  Audio.QueuePrioritizedNotification(notificationNum, 0, 10, CurrentTime);
  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Diag = %d\n", notificationNum);
    Serial.write(buf);
  }
}


void setup() {

  if (DEBUG_MESSAGES) {
    // If debug is on, set up the Serial port for communication
    Serial.begin(115200);
    Serial.write("Starting\n");
  }

  // Set up the Audio handler in order to play boot messages
  CurrentTime = millis();
  if (DEBUG_MESSAGES) Serial.write("Staring Audio\n");
  Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  Audio.StopAllAudio();
  Audio.SetMusicDuckingGain(20);
  Audio.SetSoundFXDuckingGain(15);

  // Set up the chips and interrupts
  unsigned long initResult = 0;
  if (DEBUG_MESSAGES) Serial.write("Initializing MPU\n");
  initResult = RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_INIT_AND_RETURN_EVEN_IF_ORIGINAL_CHOSEN | RPU_CMD_PERFORM_MPU_TEST, SW_CREDIT_RESET);

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Return from init = 0x%04lX\n", initResult);
    Serial.write(buf);
    if (initResult & RPU_RET_6800_DETECTED) Serial.write("Detected 6800 clock\n");
    else if (initResult & RPU_RET_6802_OR_8_DETECTED) Serial.write("Detected 6802/8 clock\n");
    Serial.write("Back from init\n");
  }

//  if (initResult & RPU_RET_SELECTOR_SWITCH_ON) QueueDIAGNotification(SOUND_EFFECT_DIAG_SELECTOR_SWITCH_ON);
//  else QueueDIAGNotification(422);
//  if (initResult & RPU_RET_CREDIT_RESET_BUTTON_HIT) QueueDIAGNotification(SOUND_EFFECT_DIAG_CREDIT_RESET_BUTTON);

  if (initResult & RPU_RET_DIAGNOSTIC_REQUESTED) {
    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_DIAGNOSTICS);
    // Run diagnostics here:
  }

  ReadStoredParameters();

  if (initResult & RPU_RET_ORIGINAL_CODE_REQUESTED) {
    delay(100);

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.InitOutogingCommunication();
    TopperALB.SetTargetDeviceAddress(8);
    TopperALB.EnableLamps();
    TopperALB.WatchdogTimerReset();
    TopperALB.AdjustSetting(SET_COLOR_0, 0, 0, 200);
    TopperALB.AdjustSetting(SET_COLOR_1, 200, 200, 0);
    ALBWatchdogResetLastSent = millis();
#endif

//    QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_ORIGINAL_CODE);
    while (Audio.Update(millis()));

    // Arduino should hang if original code is running
    while (1) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      // Send a periodic message to keep GI on
      if (GIInOriginal) {
        if (millis() >  (ALBWatchdogResetLastSent + ALB_WATCHDOG_TIMER_DURATION/2)) {
          TopperALB.WatchdogTimerReset();
          ALBWatchdogResetLastSent = millis();
        }
      }
#endif
    }
  }
  //QueueDIAGNotification(SOUND_EFFECT_DIAG_STARTING_NEW_CODE);
  Audio.PlaySoundCardWhenPossible(19, CurrentTime, 0, 10, 10);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Read parameters from EEProm
  RPU_SetCoinLockout((FreePlayMode || Credits >= MaximumCredits) ? true : false, SOLCONT_COIN_LOCKOUT);

  for (byte count=0; count<4; count++) {
    CurrentScores[count] = 0;
    CurrentAchievements[count] = 0;
  }

  DropTargetsUL.DefineSwitch(0, SW_UL_DROP_1);
  DropTargetsUL.DefineSwitch(1, SW_UL_DROP_2);
  DropTargetsUL.DefineSwitch(2, SW_UL_DROP_3);
  DropTargetsUL.DefineResetSolenoid(0, SOL_UL_DROP_RESET);

  DropTargetsUR.DefineSwitch(0, SW_UR_DROP_1);
  DropTargetsUR.DefineSwitch(1, SW_UR_DROP_2);
  DropTargetsUR.DefineSwitch(2, SW_UR_DROP_3);
  DropTargetsUR.DefineResetSolenoid(0, SOL_UR_DROP_RESET);

  DropTargetsLL.DefineSwitch(0, SW_LL_DROP_1);
  DropTargetsLL.DefineSwitch(1, SW_LL_DROP_2);
  DropTargetsLL.DefineSwitch(2, SW_LL_DROP_3);
  DropTargetsLL.DefineResetSolenoid(0, SOL_LL_DROP_RESET);

  DropTargetsLR.DefineSwitch(0, SW_LR_DROP_1);
  DropTargetsLR.DefineSwitch(1, SW_LR_DROP_2);
  DropTargetsLR.DefineSwitch(2, SW_LR_DROP_3);
  DropTargetsLR.DefineResetSolenoid(0, SOL_LR_DROP_RESET);

  //NumberOfComboDefinitions = InitBKCombosArray();

  //Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  //Audio.StopAllAudio();
  //delay(10);
  //Audio.QueueSound(SOUND_EFFECT_HORSE_NEIGHING, AUDIO_PLAY_TYPE_WAV_TRIGGER, CurrentTime+1200);


  GIReturnState = true;
  for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
    GIOverrideOffTime[count] = 0;
    GIOverrideHoldTime[count] = 0;
  }


  OperatorSwitchPressStarted = 0;
  InOperatorMenu = false;
  Menus.SetNavigationButtons(SW_RIGHT_MAGNET_BUTTON, SW_LEFT_MAGNET_BUTTON, SW_CREDIT_RESET, SW_SELF_TEST_SWITCH);
  Menus.SetMenuButtonDebounce(500);
  Menus.SetLampsLookupCallback(LampConvertDisplayNumberToIndex);
  Menus.SetSolenoidIDLookupCallback(SolenoidConvertDisplayNumberToIndex);
  Menus.SetSolenoidStrengthLookupCallback(SolenoidConvertDisplayNumberToTestStrength);

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
  TopperALB.InitOutogingCommunication();
  TopperALB.SetTargetDeviceAddress(8);
  TopperALB.EnableLamps();
  TopperALB.WatchdogTimerReset();
  TopperALB.AdjustSetting(SET_COLOR_0, 0, 0, 200);
  TopperALB.AdjustSetting(SET_COLOR_1, 200, 200, 0);
  ALBWatchdogResetLastSent = millis();
#endif

  LastTiltWarningTime = 0;
}

byte ReadSetting(byte setting, byte defaultValue, byte maxValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF || value>maxValue) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (RPU_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////

void ShowLockLamps() {

  if (GameMode == GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime - GameModeStartTime) / 500) % 3;
    RPU_SetLampState(LAMP_LOCK_1, lampPhase == 2);
    RPU_SetLampState(LAMP_LOCK_2, lampPhase == 1);
    RPU_SetLampState(LAMP_LOCK_3, lampPhase == 0);
  } else if (PlayLockKickWarningTime && (CurrentTime+1000)>PlayLockKickWarningTime) {
    RPU_SetLampState(LAMP_LOCK_1, 1, 0, 25);
    RPU_SetLampState(LAMP_LOCK_2, 1, 0, 25);
    RPU_SetLampState(LAMP_LOCK_3, 1, 0, 25);
  } else if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_LOCK_JACKPOT) {
    RPU_SetLampState(LAMP_LOCK_1, 1, 0, 75);
    RPU_SetLampState(LAMP_LOCK_2, 1, 0, 75);
    RPU_SetLampState(LAMP_LOCK_3, 1, 0, 75);
  } else if (LockLitFromCombo) {
    byte lampPhase = (CurrentTime / 50) % 3;
    RPU_SetLampState(LAMP_LOCK_1, lampPhase == 0);
    RPU_SetLampState(LAMP_LOCK_2, lampPhase == 1);
    RPU_SetLampState(LAMP_LOCK_3, lampPhase == 2);
  } else if (KingsChallengeStatus[CurrentPlayer] & KINGS_CHALLENGE_AVAILABLE) {
    byte kcVersusLockPhase = (CurrentTime/1600)%2;
    if (kcVersusLockPhase==0) {
      byte lampPhase = (CurrentTime/100)%8;
      RPU_SetLampState(LAMP_LOCK_1, lampPhase==0);
      RPU_SetLampState(LAMP_LOCK_2, lampPhase==0);
      RPU_SetLampState(LAMP_LOCK_3, lampPhase==0);
    } else {
      RPU_SetLampState(LAMP_LOCK_1, PlayerLockStatus[CurrentPlayer] & (LOCK_1_ENGAGED | LOCK_1_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_1_AVAILABLE) ? 200 : 0);
      RPU_SetLampState(LAMP_LOCK_2, PlayerLockStatus[CurrentPlayer] & (LOCK_2_ENGAGED | LOCK_2_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_2_AVAILABLE) ? 200 : 0);
      RPU_SetLampState(LAMP_LOCK_3, PlayerLockStatus[CurrentPlayer] & (LOCK_3_ENGAGED | LOCK_3_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_3_AVAILABLE) ? 200 : 0);
    }
  } else if (GameMode == GAME_MODE_DOUBLE_COMBAT && CombatJackpotReady) {
    RPU_SetLampState(LAMP_LOCK_1, 1, 0, 230);
    RPU_SetLampState(LAMP_LOCK_2, 1, 0, 230);
    RPU_SetLampState(LAMP_LOCK_3, 1, 0, 230);
  } else if (GameMode == GAME_MODE_TRIPLE_COMBAT && CombatJackpotReady) {
    RPU_SetLampState(LAMP_LOCK_1, 1, 0, 130);
    RPU_SetLampState(LAMP_LOCK_2, 1, 0, 130);
    RPU_SetLampState(LAMP_LOCK_3, 1, 0, 130);
  } else if (GameMode == GAME_MODE_UNSTRUCTURED_PLAY) {
    RPU_SetLampState(LAMP_LOCK_1, PlayerLockStatus[CurrentPlayer] & (LOCK_1_ENGAGED | LOCK_1_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_1_AVAILABLE) ? 200 : 0);
    RPU_SetLampState(LAMP_LOCK_2, PlayerLockStatus[CurrentPlayer] & (LOCK_2_ENGAGED | LOCK_2_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_2_AVAILABLE) ? 200 : 0);
    RPU_SetLampState(LAMP_LOCK_3, PlayerLockStatus[CurrentPlayer] & (LOCK_3_ENGAGED | LOCK_3_AVAILABLE), 0, (PlayerLockStatus[CurrentPlayer]&LOCK_3_AVAILABLE) ? 200 : 0);
  } else {
    RPU_SetLampState(LAMP_LOCK_1, 0);
    RPU_SetLampState(LAMP_LOCK_2, 0);
    RPU_SetLampState(LAMP_LOCK_3, 0);
  }

}


void ShowBonusLamps() {
  if (GameMode == GAME_MODE_SKILL_SHOT) {
    for (byte count = LAMP_BONUS_1; count <=  LAMP_BONUS_40; count++) RPU_SetLampState(count, 0);
  } else {
    byte bonusToShow = Bonus[CurrentPlayer];
    if (BonusAnimationStart) {
      bonusToShow = 1 + ((CurrentTime - BonusAnimationStart) / 25);
      if (bonusToShow >= Bonus[CurrentPlayer]) {
        BonusAnimationStart = 0;
        bonusToShow = Bonus[CurrentPlayer];
      }
    }

    RPU_SetLampState(LAMP_BONUS_40, (bonusToShow >= 40));
    if (bonusToShow >= 40) bonusToShow -= 40;
    RPU_SetLampState(LAMP_BONUS_30, (bonusToShow >= 30));
    if (bonusToShow >= 30) bonusToShow -= 30;
    RPU_SetLampState(LAMP_BONUS_20, (bonusToShow >= 20));
    if (bonusToShow >= 20) bonusToShow -= 20;
    RPU_SetLampState(LAMP_BONUS_10, (bonusToShow >= 10));
    if (bonusToShow >= 10) bonusToShow -= 10;

    byte effectiveOnes = bonusToShow % 10;
    if (bonusToShow != 0 && effectiveOnes == 0) effectiveOnes = 10;
    for (byte count = 1; count < 10; count++) RPU_SetLampState(LAMP_BONUS_1 + (count - 1), effectiveOnes >= count);
  }
}

void ShowBonusXLamps() {

  if (BonusXCollectAvailableStart) {

    if (CurrentTime > (BonusXCollectAvailableStart + 1500)) {
      BonusXCollectAvailableStart = 0;
    }
    boolean lampDim = (((CurrentTime - BonusXCollectAvailableStart) % 500) / 166) ? true : false;
    boolean lampOn = (((CurrentTime - BonusXCollectAvailableStart) % 500) / 333) ? false : true;

    RPU_SetLampState(LAMP_2X, lampOn && (BonusX[CurrentPlayer] == 1 || BonusX[CurrentPlayer] == 5 || BonusX[CurrentPlayer] == 6), lampDim);
    RPU_SetLampState(LAMP_3X, lampOn && (BonusX[CurrentPlayer] == 2 || BonusX[CurrentPlayer] == 7), lampDim);
    RPU_SetLampState(LAMP_4X, lampOn && (BonusX[CurrentPlayer] == 3 || BonusX[CurrentPlayer] == 5 || BonusX[CurrentPlayer] == 8), lampDim);
    RPU_SetLampState(LAMP_5X, lampOn && (BonusX[CurrentPlayer] == 4 || BonusX[CurrentPlayer] >= 6), lampDim);
  } else {

    int flashVal = 0;

    if (BonusXAnimationStart) {
      if (CurrentTime > (BonusXAnimationStart + 3000)) BonusXAnimationStart = 0;
      flashVal = 150;
    }

    if (flashVal == 0 && BonusX[CurrentPlayer] > 5) flashVal = 400;

    RPU_SetLampState(LAMP_2X, BonusX[CurrentPlayer] == 2 || BonusX[CurrentPlayer] == 6 || BonusX[CurrentPlayer] == 7, 0, flashVal);
    RPU_SetLampState(LAMP_3X, BonusX[CurrentPlayer] == 3 || BonusX[CurrentPlayer] == 8, 0, flashVal);
    RPU_SetLampState(LAMP_4X, BonusX[CurrentPlayer] == 4 || BonusX[CurrentPlayer] == 6 || BonusX[CurrentPlayer] == 9, 0, flashVal);
    RPU_SetLampState(LAMP_5X, BonusX[CurrentPlayer] == 5 || BonusX[CurrentPlayer] >= 7, 0, flashVal);
  }
}


void ShowPlayfieldXAndMagnetLamps() {

  if (KingsChallengeRunning & KINGS_CHALLENGE_LEVITATE) {
    RPU_SetLampState(LAMP_LEFT_MAGNASAVE, LevitateMagnetOffTime ? 1 : 0, 0, 75);
    RPU_SetLampState(LAMP_RIGHT_MAGNASAVE, 0);
  } else if (!MagnaSaveAvailable) {
    RPU_SetLampState(LAMP_LEFT_MAGNASAVE, 0);
    RPU_SetLampState(LAMP_RIGHT_MAGNASAVE, 0);
  } else {
    int leftFlash, rightFlash;
    if (MagnaStatusLeft[CurrentPlayer] <= 1000) leftFlash = 0;
    else leftFlash = 1000 - MagnaStatusLeft[CurrentPlayer] / 10;
    if (MagnaStatusRight[CurrentPlayer] <= 1000) rightFlash = 0;
    else rightFlash = 1000 - MagnaStatusRight[CurrentPlayer] / 10;
    RPU_SetLampState(LAMP_LEFT_MAGNASAVE, MagnaStatusLeft[CurrentPlayer], 0, leftFlash);
    RPU_SetLampState(LAMP_RIGHT_MAGNASAVE, MagnaStatusRight[CurrentPlayer], 0, rightFlash);
  }

  if (PlayfieldMultiplier == 1) {
    RPU_SetLampState(LAMP_DOUBLE_SCORING, 0);
    RPU_SetLampState(LAMP_TRIPLE_SCORING, 0);
  } else {
    RPU_SetLampState(LAMP_DOUBLE_SCORING, PlayfieldMultiplier == 2 || PlayfieldMultiplier > 3, 0, (PlayfieldMultiplier == 4) ? 250 : 0);
    RPU_SetLampState(LAMP_TRIPLE_SCORING, PlayfieldMultiplier == 3 || PlayfieldMultiplier == 5);
  }
}


void ShowSpinnerAndPopBumperLamp() {
  //LAMP_SPINNER
  //LAMP_POP_BUMPER

  if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_SPINNER_JACKPOT) {
    RPU_SetLampState(LAMP_SPINNER, 1, 0, 75);
  } else {
    RPU_SetLampState(LAMP_SPINNER, SpinnerStatus, 0, (CurrentTime > (SpinnerLitUntil - 1000)) ? 100 : 500);
  }

  if (KingsChallengeRunning & KINGS_CHALLENGE_MELEE) {
    RPU_SetLampState(LAMP_POP_BUMPER, 1, 0, LastPopBumperHit ? 75 : 500);
  } else {
    RPU_SetLampState(LAMP_POP_BUMPER, 1, 0, LastPopBumperHit ? 75 : 0);
  }
}


void ShowHeadLamps() {
  //RPU_SetLampState(LAMP_HEAD_TIMER_BONUS_BALL
  //RPU_SetLampState(LAMP_HEAD_BALL_IN_PLAY
}


void ShowLaneAndRolloverLamps() {

  boolean ballReady = ((LastChanceAvailable && (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK)) ? true : false);

  RPU_SetLampState(LAMP_LEFT_OUTLANE, ballReady && ((LastChanceStatus[CurrentPlayer]&LAST_CHANCE_LEFT_QUALIFIED) ? true : false), 0, (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK) ? 0 : 100);
  RPU_SetLampState(LAMP_RIGHT_OUTLANE, ballReady && ((LastChanceStatus[CurrentPlayer]&LAST_CHANCE_RIGHT_QUALIFIED) ? true : false), 0, (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK) ? 0 : 100);

  if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_MIDDLE_RAMP_JACKPOT) {
    RPU_SetLampState(LAMP_MIDDLE_RAMP, 1, 0, 75);
  } else {
    RPU_SetLampState(LAMP_MIDDLE_RAMP, RightRampLitFromCombo, 0, 50);
  }
  if (RightInlaneLitFromCombo) RPU_SetLampState(LAMP_RIGHT_INLANE, 1, 0, 50);
  else RPU_SetLampState(LAMP_RIGHT_INLANE, LastTimeRightMagnetOn ? true : false, 0, 75);

  if (LeftInlaneLitFromCombo) RPU_SetLampState(LAMP_LEFT_INLANE, 1, 0, 50);
  else RPU_SetLampState(LAMP_LEFT_INLANE, LastTimeLeftMagnetOn ? true : false, 0, 75);

  // Saucer Lamp

  if (SaucerKickTime) {
    if ( (CurrentTime+2000) > SaucerKickTime ) {
      RPU_SetLampState(LAMP_SAUCER, 1, 0, 50);
    } else {
      byte lampPhase = (CurrentTime/200)%4;
      RPU_SetLampState(LAMP_SAUCER, lampPhase, (lampPhase%2), 0);
    }
  } else if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_SAUCER_JACKPOT) {
    RPU_SetLampState(LAMP_SAUCER, 1, 0, 75);
  } else if (BonusXCollectAvailableStart) {
    boolean lampDim = (((CurrentTime - BonusXCollectAvailableStart) % 500) / 166) ? true : false;
    boolean lampOn = (((CurrentTime - BonusXCollectAvailableStart) % 500) / 333) ? false : true;
    RPU_SetLampState(LAMP_SAUCER, lampOn, lampDim);
  } else if (GameMode == GAME_MODE_SINGLE_COMBAT && CombatJackpotReady) {
    RPU_SetLampState(LAMP_SAUCER, 1, 0, 50);
  } else if (SaucerLitFromCombo) {
    byte lampPhase = ((CurrentTime / 50) % 2);
    RPU_SetLampState(LAMP_SAUCER, lampPhase == 0);
  } else if (KingsChallengeStatus[CurrentPlayer] & KINGS_CHALLENGE_AVAILABLE) {
    byte lampPhase = (CurrentTime/100)%8;
    RPU_SetLampState(LAMP_SAUCER, lampPhase == 0);
  } else {
    RPU_SetLampState(LAMP_SAUCER, 0);
  }

  if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_LOOP_JACKPOT) {
    RPU_SetLampState(LAMP_LOOP_SPECIAL, 1, 0, 75);
    RPU_SetLampState(LAMP_LOOP_EXTRA_BALL, 1, 0, 75);
  } else if (LoopLitFromCombo) {
    RPU_SetLampState(LAMP_LOOP_SPECIAL, 1, 0, 50);
    RPU_SetLampState(LAMP_LOOP_EXTRA_BALL, 1, 0, 50);
  } else if (LoopLitToQualifyLock) {
    byte lampPhase = (CurrentTime / 150) % 2;
    RPU_SetLampState(LAMP_LOOP_SPECIAL, lampPhase == 0);
    RPU_SetLampState(LAMP_LOOP_EXTRA_BALL, lampPhase == 1);
  } else {
    RPU_SetLampState(LAMP_LOOP_SPECIAL, 0);
    RPU_SetLampState(LAMP_LOOP_EXTRA_BALL, 0);
  }

  if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_UPPER_RAMP_JACKPOT) {
    RPU_SetLampState(LAMP_UPPER_EXTRA_BALL, 1, 0, 75);
  } else {
    if (UpperLeftRolloverLitFromCombo) RPU_SetLampState(LAMP_UPPER_EXTRA_BALL, 1, 0, 50);
    else RPU_SetLampState(LAMP_UPPER_EXTRA_BALL, ExtraBallsCollected<MaxExtraBallsPerBall && (ExtraBallsOrSpecialAvailable[CurrentPlayer]&EBS_UPPER_EXTRA_BALL_AVAILABLE));
  }

}


byte DropLamps[4] = {LAMP_UPPER_LEFT_DROPS, LAMP_UPPER_RIGHT_DROPS, LAMP_LOWER_LEFT_DROPS, LAMP_LOWER_RIGHT_DROPS};

void ShowDropTargetLamps() {

  byte dropPhase = (CurrentTime/1000)%2;

  if ((KingsChallengeRunning&KINGS_CHALLENGE_DROPS) && dropPhase) {
    byte dropState[4][3];
    for (byte bank=0; bank<4; bank++) {
      for (byte dropNum=0; dropNum<3; dropNum++) {
        dropState[bank][dropNum] = 0;
      }
    }
    
    if (KingsChallengeRunning & KINGS_CHALLENGE_LEVITATE) {
      dropState[2][0] = 1;
      dropState[2][1] = 1;
      dropState[2][2] = 1;
      RPU_SetLampState(DropLamps[2], 1, 0, 100);
    } else {
      RPU_SetLampState(DropLamps[2], 0);
    }
    RPU_SetLampState(DropLamps[0], 0);
    RPU_SetLampState(DropLamps[1], 0);
    RPU_SetLampState(DropLamps[3], 0);

    if (KingsChallengeRunning & KINGS_CHALLENGE_PERFECTION) {
      dropState[KingsChallengePerfectionBank][0] = 1;
      dropState[KingsChallengePerfectionBank][1] = 1;
      dropState[KingsChallengePerfectionBank][2] = 1;
    }
    if (KingsChallengeRunning & KINGS_CHALLENGE_JOUST && CurrentTime>(LastSwitchHitTime+3000)) {
      dropState[0][1] = 1;
      dropState[1][1] = 1;
      dropState[2][1] = 1;
      dropState[3][1] = 1;
    }
    for (byte count = 0; count < 3; count++) {
      RPU_SetLampState(LAMP_ULDROPS_3 + count, dropState[0][count], 0, 200);
      RPU_SetLampState(LAMP_URDROPS_3 + count, dropState[1][count], 0, 200);
      RPU_SetLampState(LAMP_LLDROPS_1 + count, dropState[2][count], 0, 200);
      RPU_SetLampState(LAMP_LRDROPS_3 + count, dropState[3][count], 0, 200);
    }    
  } else {
  
    if (GameMode == GAME_MODE_SKILL_SHOT) {
      RPU_SetLampState(LAMP_ULDROPS_1, SkillShotTarget == 0, 0, 250);
      RPU_SetLampState(LAMP_ULDROPS_2, SkillShotTarget == 1, 0, 250);
      RPU_SetLampState(LAMP_ULDROPS_3, SkillShotTarget == 2, 0, 250);
  
      for (byte count = 0; count < 3; count++) {
        RPU_SetLampState(LAMP_URDROPS_3 + count, 0);
        RPU_SetLampState(LAMP_LLDROPS_1 + count, 0);
        RPU_SetLampState(LAMP_LRDROPS_3 + count, 0);
      }
      RPU_SetLampState(LAMP_UPPER_LEFT_DROPS, 0);
      RPU_SetLampState(LAMP_UPPER_RIGHT_DROPS, 0);
      RPU_SetLampState(LAMP_LOWER_LEFT_DROPS, 0);
      RPU_SetLampState(LAMP_LOWER_RIGHT_DROPS, 0);
    } else if (GameMode == GAME_MODE_SINGLE_COMBAT) {
      byte curStatus0, curStatus1, curStatus2, curStatus3;
      byte lampPhase = (CurrentTime / 200) % 2;
      curStatus0 = DropTargetsUL.GetStatus(false);
      curStatus1 = DropTargetsUR.GetStatus(false);
      curStatus2 = DropTargetsLL.GetStatus(false);
      curStatus3 = DropTargetsLR.GetStatus(false);
  
      RPU_SetLampState(DropLamps[0], lampPhase == 0 && (curStatus0 & 0x07) != 0x07);
      RPU_SetLampState(DropLamps[1], lampPhase == 0 && (curStatus1 & 0x07) != 0x07);
      RPU_SetLampState(DropLamps[2], lampPhase == 0 && (curStatus2 & 0x07) != 0x07);
      RPU_SetLampState(DropLamps[3], lampPhase == 0 && (curStatus3 & 0x07) != 0x07);
  
      byte bitMask0, bitMask1;
      bitMask0 = 0x01;
      bitMask1 = 0x04;
      for (byte count = 0; count < 3; count++) {
  
        RPU_SetLampState(LAMP_ULDROPS_3 + count, lampPhase && (curStatus0 & bitMask0) == 0);
        RPU_SetLampState(LAMP_URDROPS_3 + count, lampPhase && (curStatus1 & bitMask0) == 0);
        RPU_SetLampState(LAMP_LLDROPS_1 + count, lampPhase && (curStatus2 & bitMask0) == 0);
        RPU_SetLampState(LAMP_LRDROPS_3 + count, lampPhase && (curStatus3 & bitMask1) == 0);
        bitMask0 *= 2;
        bitMask1 /= 2;
      }
  
    } else if (GameMode == GAME_MODE_OFFER_DOUBLE_COMBAT) {
      byte lampPhase = (CurrentTime / 100) % 3;
      for (byte count = 0; count < 3; count++) {
        RPU_SetLampState(LAMP_ULDROPS_3 + count, count == lampPhase);
        RPU_SetLampState(LAMP_URDROPS_3 + count, count == lampPhase);
        RPU_SetLampState(LAMP_LLDROPS_1 + count, count == lampPhase);
        RPU_SetLampState(LAMP_LRDROPS_3 + count, count == lampPhase);
      }
    } else if (GameMode == GAME_MODE_DOUBLE_COMBAT_START) {
      byte lampPhase = (CurrentTime / 75) % 4;
      for (byte count = 0; count < 3; count++) {
        RPU_SetLampState(LAMP_ULDROPS_3 + count, lampPhase, lampPhase % 2);
        RPU_SetLampState(LAMP_URDROPS_3 + count, lampPhase, lampPhase % 2);
        RPU_SetLampState(LAMP_LLDROPS_1 + count, lampPhase, lampPhase % 2);
        RPU_SetLampState(LAMP_LRDROPS_3 + count, lampPhase, lampPhase % 2);
      }
    } else if (GameMode == GAME_MODE_DOUBLE_COMBAT) {
      if ( (CombatBankFlags & 0x03) == 0x00 ) {
        byte lampPhase = (CurrentTime / 75) % 10;
        RPU_SetLampState(LAMP_ULDROPS_3, lampPhase == 0);
        RPU_SetLampState(LAMP_ULDROPS_2, lampPhase == 1 || lampPhase == 9);
        RPU_SetLampState(LAMP_ULDROPS_1, lampPhase == 2 || lampPhase == 8);
        RPU_SetLampState(LAMP_URDROPS_1, lampPhase == 3 || lampPhase == 7);
        RPU_SetLampState(LAMP_URDROPS_2, lampPhase == 4 || lampPhase == 6);
        RPU_SetLampState(LAMP_URDROPS_3, lampPhase == 5);
      } else {
        byte lampPhase = (CurrentTime / 400) % 4;
        for (byte count = 0; count < 3; count++) {
          RPU_SetLampState(LAMP_ULDROPS_3 + count, lampPhase && (CombatBankFlags & DROP_BANK_UL_FLAG), lampPhase % 2);
          RPU_SetLampState(LAMP_URDROPS_3 + count, lampPhase && (CombatBankFlags & DROP_BANK_UR_FLAG), lampPhase % 2);
        }
      }
      if ( (CombatBankFlags & 0x0C) == 0x00 ) {
        byte lampPhase = (CurrentTime / 75) % 10;
        RPU_SetLampState(LAMP_LLDROPS_1, lampPhase == 0);
        RPU_SetLampState(LAMP_LLDROPS_2, lampPhase == 1 || lampPhase == 9);
        RPU_SetLampState(LAMP_LLDROPS_3, lampPhase == 2 || lampPhase == 8);
        RPU_SetLampState(LAMP_LRDROPS_1, lampPhase == 3 || lampPhase == 7);
        RPU_SetLampState(LAMP_LRDROPS_2, lampPhase == 4 || lampPhase == 6);
        RPU_SetLampState(LAMP_LRDROPS_3, lampPhase == 5);
      } else {
        byte lampPhase = (CurrentTime / 400) % 4;
        for (byte count = 0; count < 3; count++) {
          RPU_SetLampState(LAMP_LLDROPS_1 + count, lampPhase && (CombatBankFlags & DROP_BANK_LL_FLAG), lampPhase % 2);
          RPU_SetLampState(LAMP_LRDROPS_3 + count, lampPhase && (CombatBankFlags & DROP_BANK_LR_FLAG), lampPhase % 2);
        }
      }
  
      byte banksLeftCount = 4 - CountBits(CombatBankFlags);
      if (banksLeftCount) {
        byte dropBankFlag = 0x01;
        byte lampPhase = (CurrentTime / 500) % banksLeftCount;
        byte litBank = 0;
        for (byte count = 0; count < 4; count++) {
          if ((CombatBankFlags & dropBankFlag) == 0x00) {
            RPU_SetLampState(DropLamps[count], litBank == lampPhase);
            litBank += 1;
          } else {
            RPU_SetLampState(DropLamps[count], 0);
          }
          dropBankFlag *= 2;
        }
      } else {
        for (byte count = 0; count < 4; count++) {
          RPU_SetLampState(DropLamps[count], 0);
        }
      }
  
  
    } else {
      for (byte count = 0; count < 4; count++) {
        if (DropTargetResetTime[count]) {
          RPU_SetLampState(DropLamps[count], DropTargetHurryLamp[count]);
        } else {
          RPU_SetLampState(DropLamps[count], 0);
        }
      }
  
      for (byte count = 0; count < 3; count++) {
        RPU_SetLampState(LAMP_ULDROPS_3 + count, NumDropTargetClears[CurrentPlayer][0] > count);
        RPU_SetLampState(LAMP_URDROPS_3 + count, NumDropTargetClears[CurrentPlayer][1] > count);
        RPU_SetLampState(LAMP_LLDROPS_1 + count, NumDropTargetClears[CurrentPlayer][2] > count);
        RPU_SetLampState(LAMP_LRDROPS_3 + count, NumDropTargetClears[CurrentPlayer][3] > count);
      }
    }
  }
}


void ShowShootAgainLamps() {

  if ( (BallFirstSwitchHitTime == 0 && BallSaveNumSeconds) || (BallSaveEndTime && CurrentTime < BallSaveEndTime) ) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime != 0) msRemaining = BallSaveEndTime - CurrentTime;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
    RPU_SetLampState(LAMP_HEAD_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    RPU_SetLampState(LAMP_HEAD_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}


void SetGeneralIlluminationOn(boolean generalIlluminationOn = true) {
  GIReturnState = generalIlluminationOn;
  for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
    if (GIOverrideOffTime[count] && CurrentTime>GIOverrideOffTime[count]) return;
  }
  RPU_SetContinuousSolenoid(!generalIlluminationOn, SOL_GI_RELAY);
}

void OverrideGeneralIllumination(unsigned long illuminationOffTime, unsigned short holdOffTime) {
  for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
    if (GIOverrideOffTime[count]==0) {
      GIOverrideOffTime[count] = illuminationOffTime;
      GIOverrideHoldTime[count] = holdOffTime;
      return;
    }
  }
}

void UpdateGI() {
  for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
    if (GIOverrideOffTime[count]) {
      if (CurrentTime>GIOverrideOffTime[count]) {
        if (CurrentTime>(GIOverrideOffTime[count] + (unsigned long)GIOverrideHoldTime[count])) {
          GIOverrideOffTime[count] = 0;
          GIOverrideHoldTime[count] = 0;
          SetGeneralIlluminationOn(GIReturnState);
        } else {
          RPU_SetContinuousSolenoid(true, SOL_GI_RELAY);
        }
      } 
    }
  }
}

void ResetGIHolds() {
  GIReturnState = true;
  for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
    GIOverrideOffTime[count] = 0;
    GIOverrideHoldTime[count] = 0;
  }
}


////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  RPU_SetLampState(LAMP_APRON_CREDITS, (Credits || FreePlayMode));
  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0, true, 2, true);
  //  RPU_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  //  RPU_SetLampState(LAMP_HEAD_1_PLAYER, CurrentNumPlayers==1, 0, 500);
  //  RPU_SetLampState(LAMP_HEAD_2_PLAYERS, CurrentNumPlayers==2, 0, 500);
  //  RPU_SetLampState(LAMP_HEAD_3_PLAYERS, CurrentNumPlayers==3, 0, 500);
  //  RPU_SetLampState(LAMP_HEAD_4_PLAYERS, CurrentNumPlayers==4, 0, 500);

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true, SOLCONT_COIN_LOCKOUT);
  }
  if (CurrentNumPlayers == 1) Audio.StopAllAudio();
  QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 10);

  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}


unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum > 2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      //PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
      RPU_PushToSolenoidStack(SOL_BELL, 40, true);
    }
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false, SOLCONT_COIN_LOCKOUT);
  } else {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true, SOLCONT_COIN_LOCKOUT);
  }

  RPU_SetLampState(LAMP_APRON_CREDITS, (Credits || FreePlayMode));

}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit == SW_COIN_2) chuteNum = 1;
  else if (switchHit == SW_COIN_3) chuteNum = 2;
  return chuteNum;
}

boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum > 2) return false;
  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse = 0; chuteNumToUse <= chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse) == cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_COIN_DROP_1 + (CurrentTime % 3));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse] == cpcCoins) {
    if (cpcCredits > cpcCoins) AddCredit(cpcCredits - (coinProgressBefore));
    else AddCredit(cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits > cpcCoins) {
      AddCredit(1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}


void AddSpecialCredit() {
  AddCredit(false, 1);
  RPU_PushToTimedSolenoidStack(SOL_BELL, 50, CurrentTime, true);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += SpecialValue * PlayfieldMultiplier;
  } else {
    AddSpecialCredit();
  }
}

boolean AwardExtraBall() {
  if (ExtraBallsCollected>=MaxExtraBallsPerBall) return false;
  ExtraBallsCollected += 1;
  ExtraBallAwardStartTime = CurrentTime;
  RPU_PushToTimedSolenoidStack(SOL_BELL, 35, CurrentTime, true);
  RPU_PushToTimedSolenoidStack(SOL_BELL, 35, CurrentTime + 400, true);
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ExtraBallValue * PlayfieldMultiplier;
  } else {
    SamePlayerShootsAgain = true;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
    RPU_SetLampState(LAMP_HEAD_SHOOT_AGAIN, SamePlayerShootsAgain);
    QueueNotification(SOUND_EFFECT_VP_EXTRA_BALL, 8);
  }
  return true;
}


void IncreasePlayfieldMultiplier(unsigned long duration) {
  if (PlayfieldMultiplierExpiration) PlayfieldMultiplierExpiration += duration;
  else PlayfieldMultiplierExpiration = CurrentTime + duration;
  PlayfieldMultiplier += 1;
  if (PlayfieldMultiplier > 5) {
    PlayfieldMultiplier = 5;
  } else {
    QueueNotification(SOUND_EFFECT_VP_RETURN_TO_1X + (PlayfieldMultiplier - 1), 1);
  }
}

unsigned long UpperLockSwitchDownTime[3] = {0, 0, 0};
unsigned long UpperLockSwitchUpTime[3] = {0, 0, 0};
unsigned long UpperLockLastChecked = 0;
unsigned long MachineLockDiscrepancyTime = 0;
boolean UpperLockSwitchState[3] = {false, false, false};

byte InitializeMachineLocksBasedOnSwitches() {
  byte returnLocks = 0;

  if (RPU_ReadSingleSwitchState(SW_LOCK_1)) {
    returnLocks |= LOCK_1_ENGAGED;
    // Also update the UpperLockSwitchState variable
    UpperLockSwitchState[0] = true;
  }
  if (RPU_ReadSingleSwitchState(SW_LOCK_2)) {
    returnLocks |= LOCK_2_ENGAGED;
    // Also update the UpperLockSwitchState variable
    UpperLockSwitchState[1] = true;
  }
  if (RPU_ReadSingleSwitchState(SW_LOCK_3)) {
    returnLocks |= LOCK_3_ENGAGED;
    // Also update the UpperLockSwitchState variable
    UpperLockSwitchState[2] = true;
  }
  //  if (includeSaucerLock && RPU_ReadSingleSwitchState(SW_SAUCER)) returnLocks |= LOCK_SAUCER_ENGAGED;

  if (DEBUG_MESSAGES) {
    char buf[256];
    sprintf(buf, "Initializing Machine Locks = 0x%04X, and ULS = %d, %d, %d\n", returnLocks, UpperLockSwitchState[0], UpperLockSwitchState[1], UpperLockSwitchState[2]);
    Serial.write(buf);
  }

  return returnLocks;
}



void UpdateLockStatus() {
  boolean lockSwitchDownTransition;
  boolean lockSwitchUpTransition;

  for (byte count = 0; count < 3; count++) {
    lockSwitchDownTransition = false;
    lockSwitchUpTransition = false;

    if (RPU_ReadSingleSwitchState(SW_LOCK_1 + count)) {
      UpperLockSwitchUpTime[count] = 0;
      if (UpperLockSwitchDownTime[count] == 0) {
        UpperLockSwitchDownTime[count] = CurrentTime;
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Down: starting down counter (%lu) for %d\n", CurrentTime, count);
          Serial.write(buf);
        }
      } else if (CurrentTime > (UpperLockSwitchDownTime[count] + 750)) {
        lockSwitchDownTransition = true;
      }
    } else {
      UpperLockSwitchDownTime[count] = 0;
      if (UpperLockSwitchUpTime[count] == 0) {
        UpperLockSwitchUpTime[count] = CurrentTime;
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Up: starting up counter (%lu) for %d\n", CurrentTime, count);
          Serial.write(buf);
        }
      } else if (CurrentTime > (UpperLockSwitchUpTime[count] + 750)) {
        lockSwitchUpTransition = true;
      }
    }

    if (lockSwitchUpTransition && UpperLockSwitchState[count]) {
      // if we used to be down & now we're up
      UpperLockSwitchState[count] = false;
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "ULS - saw up on %d\n", count);
        Serial.write(buf);
      }
    } else if (lockSwitchDownTransition && !UpperLockSwitchState[count]) {
      // if we used to be up & now we're down
      UpperLockSwitchState[count] = true;
      if (DEBUG_MESSAGES) Serial.write("Saw lock sw down - handle it\n");
      HandleLockSwitch(count);
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "ULS - saw down on %d\n", count);
        Serial.write(buf);
      }
    }
  }

  boolean waitingForKick = false;
  if (LockKickTime[0] || LockKickTime[1] || LockKickTime[2]) waitingForKick = true;

  if (!waitingForKick && LockManagementInProgress == false && (UpperLockLastChecked == 0 || CurrentTime > (UpperLockLastChecked + 200))) {
    UpperLockLastChecked = CurrentTime;
    byte curUpperLock = 0;
    if (UpperLockSwitchState[0]) curUpperLock |= LOCK_1_ENGAGED;
    if (UpperLockSwitchState[1]) curUpperLock |= LOCK_2_ENGAGED;
    if (UpperLockSwitchState[2]) curUpperLock |= LOCK_3_ENGAGED;
    if ( (MachineLocks & LOCKS_ENGAGED_MASK) != curUpperLock ) {
      if (MachineLockDiscrepancyTime == 0) {
        MachineLockDiscrepancyTime = CurrentTime;
      } else if (CurrentTime > (MachineLockDiscrepancyTime + 1000)) {
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "ML (0x%0X) from ULS = %d, %d, %d\n", MachineLocks, UpperLockSwitchState[0], UpperLockSwitchState[1], UpperLockSwitchState[2]);
          Serial.write(buf);
        }
        // If "MachineLocks" has been out of sync
        // for a full second, we should re-sync it
        // to the switches
        for (byte count = 0; count < 3; count++) UpperLockSwitchState[count] = false;
        MachineLocks &= ~LOCKS_ENGAGED_MASK;
        MachineLocks |= InitializeMachineLocksBasedOnSwitches();
        NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
      }
    } else {
      MachineLockDiscrepancyTime = 0;
    }
  }

}


void UpdatePlayerLocks() {
  byte curLockEngagedStatus = PlayerLockStatus[CurrentPlayer] & LOCKS_ENGAGED_MASK;
  if (curLockEngagedStatus) {
    // Check to see if a lock has been stolen
    if ( (curLockEngagedStatus & MachineLocks) != curLockEngagedStatus ) {
      byte lockToCheck = LOCK_1_ENGAGED;
      byte lockAvail = LOCK_1_AVAILABLE;
      for (byte count = 0; count < 3; count++) {
        if ( (curLockEngagedStatus & lockToCheck) && !(MachineLocks & lockToCheck) ) {
          // If a lock has been stolen, move it from locked to available
          PlayerLockStatus[CurrentPlayer] &= ~lockToCheck;
          PlayerLockStatus[CurrentPlayer] |= lockAvail;
        }
        lockToCheck *= 2;
        lockAvail *= 2;
      }
    }
  }
}

void LockBall(byte lockIndex = 0xFF) {

  if (lockIndex == 0xFF) {
    // We need to determine which lock this is
    lockIndex = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
    MachineLocks |= (LOCK_1_ENGAGED << lockIndex);
    NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
    if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Num BIP minus 1 to %d b/c LockBall\n", NumberOfBallsInPlay);
      Serial.write(buf);
    }

    byte playerLock = CountBits(PlayerLockStatus[CurrentPlayer] & LOCKS_ENGAGED_MASK);
    PlayerLockStatus[CurrentPlayer] &= ~(LOCK_1_AVAILABLE << playerLock);
    PlayerLockStatus[CurrentPlayer] |= (LOCK_1_ENGAGED << playerLock);

    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Ball locked -- PL=0x%02X, ML=0x%02X\n", PlayerLockStatus[CurrentPlayer], MachineLocks);
      Serial.write(buf);
    }
  } else {
    PlayerLockStatus[CurrentPlayer] &= ~(LOCK_1_AVAILABLE << lockIndex);
    PlayerLockStatus[CurrentPlayer] |= (LOCK_1_ENGAGED << lockIndex);
    MachineLocks |= (LOCK_1_ENGAGED << lockIndex);
    NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
  }
}

void ReleaseLockedBall() {
  if (NumberOfBallsLocked) {
    NumberOfBallsLocked -= 1;

    RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);

    // Figure out which ball we're kicking
    for (byte count = 0; count < 3; count++) {
      if (MachineLocks & (LOCK_3_ENGAGED >> count)) {
        // remove highest MachineLock that we find and break
        MachineLocks &= ~(LOCK_3_ENGAGED >> count);
        break;
      }
    }

    if (DEBUG_MESSAGES) {
      char buf[256];
      sprintf(buf, "Releasing - Machine Locks = 0x%04X\n", MachineLocks);
      Serial.write(buf);
    }
  }
}

boolean PutBallInPlay() {
  if (NumberOfBallsLocked == 3) {
    // Need to release a locked ball
    ReleaseLockedBall();
    return false;
  } else if (CountBallsInTrough()) {
    RPU_PushToTimedSolenoidStack(SOL_BALL_RAMP_THROWER, 16, CurrentTime + 100);
    NumberOfBallsInPlay += 1;
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Num BIP +1 to %d b/c PutBallInPlay\n", NumberOfBallsInPlay);
      Serial.write(buf);
    }
    return true;
  } else {
    // No lock and no balls in trough -- error!
    return false;
  }

  return true;
}



////////////////////////////////////////////////////////////////////////////
//
//  Operator Menu
//
////////////////////////////////////////////////////////////////////////////
#define SOUND_EFFECT_OM_CPC_VALUES                  180
#define SOUND_EFFECT_OM_CRB_VALUES                  210
#define SOUND_EFFECT_OM_DIFFICULTY_VALUES           220

#define SOUND_EFFECT_AP_TOP_LEVEL_MENU_ENTRY    1700
#define SOUND_EFFECT_AP_TEST_MENU               1701
#define SOUND_EFFECT_AP_AUDITS_MENU             1702
#define SOUND_EFFECT_AP_BASIC_ADJUSTMENTS_MENU  1703
#define SOUND_EFFECT_AP_GAME_RULES_LEVEL        1704
#define SOUND_EFFECT_AP_GAME_SPECIFIC_ADJ_MENU  1705

#define SOUND_EFFECT_AP_TEST_LAMPS              1710
#define SOUND_EFFECT_AP_TEST_DISPLAYS           1711
#define SOUND_EFFECT_AP_TEST_SOLENOIDS          1712
#define SOUND_EFFECT_AP_TEST_SWITCHES           1713
#define SOUND_EFFECT_AP_TEST_SOUNDS             1714
#define SOUND_EFFECT_AP_TEST_EJECT_BALLS        1715

#define SOUND_EFFECT_AP_AUDIT_TOTAL_PLAYS       1720
#define SOUND_EFFECT_AP_AUDIT_CHUTE_1_COINS     1721
#define SOUND_EFFECT_AP_AUDIT_CHUTE_2_COINS     1722
#define SOUND_EFFECT_AP_AUDIT_CHUTE_3_COINS     1723
#define SOUND_EFFECT_AP_AUDIT_TOTAL_REPLAYS     1724
#define SOUND_EFFECT_AP_AUDIT_AVG_BALL_TIME     1725
#define SOUND_EFFECT_AP_AUDIT_HISCR_BEAT        1726
#define SOUND_EFFECT_AP_AUDIT_TOTAL_BALLS       1727
#define SOUND_EFFECT_AP_AUDIT_NUM_MATCHES       1728
#define SOUND_EFFECT_AP_AUDIT_MATCH_PERCENTAGE  1729
#define SOUND_EFFECT_AP_AUDIT_LIFETIME_PLAYS    1730
#define SOUND_EFFECT_AP_AUDIT_MINUTES_ON        1731
#define SOUND_EFFECT_AP_AUDIT_CLEAR_AUDITS      1732

#define OM_BASIC_ADJ_IDS_FREEPLAY               0
#define OM_BASIC_ADJ_IDS_BALL_SAVE              1
#define OM_BASIC_ADJ_IDS_TILT_WARNINGS          2
#define OM_BASIC_ADJ_IDS_MUSIC_VOLUME           3
#define OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME         4
#define OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME        5
#define OM_BASIC_ADJ_IDS_BALLS_PER_GAME         6
#define OM_BASIC_ADJ_IDS_TOURNAMENT_MODE        7
#define OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE       8
#define OM_BASIC_ADJ_IDS_SPECIAL_VALUE          9 
#define OM_BASIC_ADJ_IDS_RESET_DURING_GAME      10
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_1          11
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_2          12
#define OM_BASIC_ADJ_IDS_SCORE_LEVEL_3          13
#define OM_BASIC_ADJ_IDS_SCORE_AWARDS           14
#define OM_BASIC_ADJ_IDS_SCROLLING_SCORES       15
#define OM_BASIC_ADJ_IDS_HISCR                  16
#define OM_BASIC_ADJ_IDS_CREDITS                17
#define OM_BASIC_ADJ_IDS_CPC_1                  18
#define OM_BASIC_ADJ_IDS_CPC_2                  19
#define OM_BASIC_ADJ_IDS_CPC_3                  20
#define OM_BASIC_ADJ_IDS_MATCH_FEATURE          21
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
#define OM_BASIC_ADJ_IDS_TOPPER_BRIGHTNESS      22
#define OM_BASIC_ADJ_IDS_SPEAKER_BRIGHTNESS     23
#define OM_BASIC_ADJ_IDS_LEFT_RAMP_BRIGHTNESS   24
#define OM_BASIC_ADJ_IDS_RIGHT_RAMP_BRIGHTNESS  25
#define OM_BASIC_ADJ_IDS_STADIUM_BRIGHTNESS     26
#define OM_BASIC_ADJ_IDS_GI_IN_ORIGINAL         27
#define OM_BASIC_ADJ_FINISHED                   28
#else
#define OM_BASIC_ADJ_FINISHED                   22
#endif
#define SOUND_EFFECT_AP_FREEPLAY                (1740 + OM_BASIC_ADJ_IDS_FREEPLAY)
#define SOUND_EFFECT_AP_BALL_SAVE_SECONDS       (1740 + OM_BASIC_ADJ_IDS_BALL_SAVE)
#define SOUND_EFFECT_AP_TILT_WARNINGS           (1740 + OM_BASIC_ADJ_IDS_TILT_WARNINGS)
#define SOUND_EFFECT_AP_MUSIC_VOLUME            (1740 + OM_BASIC_ADJ_IDS_MUSIC_VOLUME)
#define SOUND_EFFECT_AP_SOUNDFX_VOLUME          (1740 + OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME)
#define SOUND_EFFECT_AP_CALLOUTS_VOLUME         (1740 + OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME)
#define SOUND_EFFECT_AP_BALLS_PER_GAME          (1740 + OM_BASIC_ADJ_IDS_BALLS_PER_GAME)
#define SOUND_EFFECT_AP_TOURNAMENT_MODE         (1740 + OM_BASIC_ADJ_IDS_TOURNAMENT_MODE)
#define SOUND_EFFECT_AP_EXTRA_BALL_VALUE        (1740 + OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE)
#define SOUND_EFFECT_AP_SPECIAL_VALUE           (1740 + OM_BASIC_ADJ_IDS_SPECIAL_VALUE)
#define SOUND_EFFECT_AP_RESET_DURING_GAME       (1740 + OM_BASIC_ADJ_IDS_RESET_DURING_GAME)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_1       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_1)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_2       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_2)
#define SOUND_EFFECT_AP_ADJ_SCORE_LEVEL_3       (1740 + OM_BASIC_ADJ_IDS_SCORE_LEVEL_3)
#define SOUND_EFFECT_AP_SCORE_AWARDS            (1740 + OM_BASIC_ADJ_IDS_SCORE_AWARDS)
#define SOUND_EFFECT_AP_SCROLLING_SCORES        (1740 + OM_BASIC_ADJ_SCROLLING_SCORES)
#define SOUND_EFFECT_AP_ADJ_HISCR               (1740 + OM_BASIC_ADJ_IDS_HISCR)
#define SOUND_EFFECT_AP_ADJ_CREDITS             (1740 + OM_BASIC_ADJ_IDS_CREDITS)
#define SOUND_EFFECT_AP_ADJ_CPC_1               (1740 + OM_BASIC_ADJ_IDS_CPC_1)
#define SOUND_EFFECT_AP_ADJ_CPC_2               (1740 + OM_BASIC_ADJ_IDS_CPC_2)
#define SOUND_EFFECT_AP_ADJ_CPC_3               (1740 + OM_BASIC_ADJ_IDS_CPC_3)
#define SOUND_EFFECT_AP_MATCH_FEATURE           (1740 + OM_BASIC_ADJ_IDS_MATCH_FEATURE)
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
#define SOUND_EFFECT_AP_TOPPER_BRIGHTNESS       (1740 + OM_BASIC_ADJ_IDS_TOPPER_BRIGHTNESS)
#define SOUND_EFFECT_AP_SPEAKER_BRIGHTNESS      (1740 + OM_BASIC_ADJ_IDS_SPEAKER_BRIGHTNESS)
#define SOUND_EFFECT_AP_LEFT_RAMP_BRIGHTNESS    (1740 + OM_BASIC_ADJ_IDS_LEFT_RAMP_BRIGHTNESS)
#define SOUND_EFFECT_AP_RIGHT_RAMP_BRIGHTNESS   (1740 + OM_BASIC_ADJ_IDS_RIGHT_RAMP_BRIGHTNESS)
#define SOUND_EFFECT_AP_STADIUM_BRIGHTNESS      (1740 + OM_BASIC_ADJ_IDS_STADIUM_BRIGHTNESS)
#define SOUND_EFFECT_AP_GI_IN_ORIGINAL          (1740 + OM_BASIC_ADJ_IDS_GI_IN_ORIGINAL)
#endif

#define SOUND_EFFECT_OM_EASY_RULES_INSTRUCTIONS           1770
#define SOUND_EFFECT_OM_MEDIUM_RULES_INSTRUCTIONS         1771
#define SOUND_EFFECT_OM_HARD_RULES_INSTRUCTIONS           1772
#define SOUND_EFFECT_OM_PROGRESSIVE_RULES_INSTRUCTIONS    1773
#define SOUND_EFFECT_OM_CUSTOM_RULES_INSTRUCTIONS         1774

#define OM_GAME_ADJ_EASY_DIFFICULTY                 0
#define OM_GAME_ADJ_MEDIUM_DIFFICULTY               1
#define OM_GAME_ADJ_HARD_DIFFICULTY                 2
#define OM_GAME_ADJ_PROGRESSIVE_DIFFICULTY          3
#define OM_GAME_ADJ_CUSTOM_DIFFICULTY               4
#define SOUND_EFFECT_AP_DIFFICULTY                  (1790 + OM_GAME_ADJ_EASY_DIFFICULTY)

#define OM_GAME_ADJ_DROP_TARGET_HURRYUP_TIME        0
#define OM_SINGLE_COMBAT_NUMBER_OF_SECONDS          1
#define OM_BALL_SAVE_ON_SINGLE_COMBAT               2
#define OM_GRACE_TIME_ON_DOUBLE_COMBAT              3
#define OM_GRACE_TIME_ON_TRIPLE_COMBAT              4
#define OM_COMBOS_UNTIL_OMNIA                       5
#define OM_MAX_EXTRA_BALLS_PER_BALL                 6
#define OM_REQUIRE_PORTCULLIS_FOR_LOCKS             7
#define OM_BASE_TIME_FOR_KINGS_CHALLENGES           8
#define OM_GRACE_TIME_FOR_COMBOS                    9
#define OM_MAGNASAVE_MAX_SECONDS                    10
#define OM_SPINS_FOR_BONUS_X                        11
#define OM_GAME_ADJ_FINISHED                        12
#define SOUND_EFFECT_AP_DT_HURRYUP_TIME             (1800 + OM_GAME_ADJ_DROP_TARGET_HURRYUP_TIME)


#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
byte ConvertALBrightnessToAdjustmentLevel(byte brightness) {
  return brightness / 25;
}

byte ConvertAdjustmentLevelToALBBrightness(byte level) {
  if (level>10) return 250;
  return level * 25;
}
#endif

unsigned long SoundSettingTimeout;
unsigned long SoundTestStart;
unsigned long LastSaucerEjectTime = 0;
unsigned long LastUpperLockEjectTime = 0;
byte SoundTestSequence;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
byte ALBCurrentBrightness;
#endif
  
void RunOperatorMenu() {
  if (!Menus.UpdateMenu(CurrentTime)) {
    // Menu is done
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.StopAnimation();
#endif    
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    Audio.StopAllAudio();
    RPU_TurnOffAllLamps();
    if (MachineState==MACHINE_STATE_ATTRACT) {
      RPU_SetDisplayBallInPlay(0, true);
    } else {
      RPU_SetDisplayBallInPlay(CurrentBallInPlay);
    }
    SoundSettingTimeout = 0;
    return;
  }

  // It's up to this function to eject balls if requested
  if (Menus.BallEjectInProgress()) {
    if (CountBallsInTrough()) {
      if (CurrentTime > (LastTimeBallServed+1500)) {
        LastTimeBallServed = CurrentTime;
        RPU_PushToSolenoidStack(SOL_BALL_RAMP_THROWER, 16, true);
      }
    }
    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
      if (CurrentTime > (BallRampKicked + 1500)) {
        BallRampKicked = CurrentTime;
        RPU_PushToSolenoidStack(SOL_OUTHOLE, 16, true);
      }
    }    
    if (RPU_ReadSingleSwitchState(SW_LOCK_1)) {
      if (CurrentTime > (LastUpperLockEjectTime+1500)) {
        LastUpperLockEjectTime = CurrentTime;
        RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
      }
    }
    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      if (CurrentTime > (LastSaucerEjectTime+1500)) {
        LastSaucerEjectTime = CurrentTime;
        RPU_PushToSolenoidStack(SOL_SAUCER, 12, true);
      }
    }
  } else {
    LastTimeBallServed = 0;
    LastSaucerEjectTime = 0;
    LastUpperLockEjectTime = 0;
  }
  
  byte topLevel = Menus.GetTopLevel();
  byte subLevel = Menus.GetSubLevel();

  if (Menus.HasTopLevelChanged()) {
    // Play an audio prompt for the top level
    SoundTestStart = 0;
    Audio.StopAllAudio();
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.StopAnimation();
#endif    
    Audio.PlaySound((unsigned short)topLevel + SOUND_EFFECT_AP_TOP_LEVEL_MENU_ENTRY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_RULES_LEVEL) Menus.SetNumSubLevels(4);
    if (Menus.GetTopLevel()==OPERATOR_MENU_BASIC_ADJ_MENU) {
      GetCPCSelection(0); // make sure CPC values have been read
      Menus.SetNumSubLevels(OM_BASIC_ADJ_FINISHED);
    }
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_ADJ_MENU) Menus.SetNumSubLevels(OM_GAME_ADJ_FINISHED);
  }
  if (Menus.HasSubLevelChanged()) {
    SoundTestStart = 0;
    // Play an audio prompt for the sub level    
    Audio.StopAllAudio();
    if (topLevel==OPERATOR_MENU_SELF_TEST_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_TEST_LAMPS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      if (subLevel==OPERATOR_MENU_TEST_SOUNDS) {
        SoundTestStart = CurrentTime + 1000;
        SoundTestSequence = 0;
      } else {
        SoundTestStart = 0;
      }
    } else if (topLevel==OPERATOR_MENU_AUDITS_MENU) {
      unsigned long *currentAdjustmentUL = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentType = OPERATOR_MENU_AUD_CLEARABLE;

      switch (subLevel) {
        case 0:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_TOTAL_PLAYS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_PLAYS_EEPROM_START_BYTE;
          break;
        case 1:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_1_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_1_COINS_START_BYTE;
          break;
        case 2:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_2_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_2_COINS_START_BYTE;
          break;
        case 3:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_CHUTE_3_COINS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_CHUTE_3_COINS_START_BYTE;
          break;
        case 4:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_TOTAL_REPLAYS, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_REPLAYS_EEPROM_START_BYTE;
          break;
        case 5:
          Audio.PlaySound(SOUND_EFFECT_AP_AUDIT_HISCR_BEAT, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
          currentAdjustmentStorageByte = RPU_TOTAL_HISCORE_BEATEN_START_BYTE;
          break;
      }

      Menus.SetAuditControls(currentAdjustmentUL, currentAdjustmentStorageByte, adjustmentType);

    } else if (topLevel==OPERATOR_MENU_BASIC_ADJ_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_FREEPLAY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      byte *currentAdjustmentByte = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentValues[8] = {0};
      byte numAdjustmentValues = 2;
      byte adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
      short parameterCallout = 0;
      unsigned long *currentAdjustmentUL = NULL;
      
      adjustmentValues[1] = 1;

      switch(subLevel) {
        case OM_BASIC_ADJ_IDS_FREEPLAY:
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&FreePlayMode;
          currentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_BALL_SAVE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 5;
          adjustmentValues[2] = 10;
          adjustmentValues[3] = 15;
          adjustmentValues[4] = 20;
          currentAdjustmentByte = &BallSaveNumSeconds;
          currentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_TILT_WARNINGS:
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 2;
          currentAdjustmentByte = &MaxTiltWarnings;
          currentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_MUSIC_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &MusicVolume;
          currentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &SoundEffectsVolume;
          currentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &CalloutsVolume;
          currentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_BALLS_PER_GAME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          numAdjustmentValues = 8;
          adjustmentValues[0] = 3;
          adjustmentValues[1] = 10;
          currentAdjustmentByte = &BallsPerGame;
          currentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_TOURNAMENT_MODE:
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&TournamentScoring;
          currentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_EXTRA_BALL_VALUE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &ExtraBallValue;
          currentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_UL;
          break;
        case OM_BASIC_ADJ_IDS_SPECIAL_VALUE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &SpecialValue;
          currentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_UL;
          break;
        case OM_BASIC_ADJ_IDS_RESET_DURING_GAME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          adjustmentValues[2] = 2;
          adjustmentValues[3] = 3;
          adjustmentValues[4] = 99;
          currentAdjustmentByte = &TimeRequiredToResetGame;
          currentAdjustmentStorageByte = EEPROM_CRB_HOLD_TIME;
          parameterCallout = SOUND_EFFECT_OM_CRB_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_1:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[0];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_1_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_2:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[1];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_2_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_LEVEL_3:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &AwardScores[2];
          currentAdjustmentStorageByte = RPU_AWARD_SCORE_3_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCORE_AWARDS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX_DEFAULT;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 7;
          currentAdjustmentByte = &ScoreAwardReplay;
          currentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_SCROLLING_SCORES:
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&ScrollingScores;
          currentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_HISCR:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_SCORE_WITH_DEFAULT;
          currentAdjustmentUL = &HighScore;
          currentAdjustmentStorageByte = RPU_HIGHSCORE_EEPROM_START_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CREDITS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 40;
          currentAdjustmentByte = &Credits;
          currentAdjustmentStorageByte = RPU_CREDITS_EEPROM_BYTE;
          break;
        case OM_BASIC_ADJ_IDS_CPC_1:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[0]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_1_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_CPC_2:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[1]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_2_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_CPC_3:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_CPC;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = (NUM_CPC_PAIRS-1);
          currentAdjustmentByte = &(CPCSelection[2]);
          currentAdjustmentStorageByte = RPU_CPC_CHUTE_3_SELECTION_BYTE;
          parameterCallout = SOUND_EFFECT_OM_CPC_VALUES;
          break;
        case OM_BASIC_ADJ_IDS_MATCH_FEATURE:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&MatchFeature;
          currentAdjustmentStorageByte = EEPROM_MATCH_FEATURE_BYTE;
          break;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        case OM_BASIC_ADJ_IDS_TOPPER_BRIGHTNESS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          ALBCurrentBrightness = 0;
          currentAdjustmentByte = &ALBCurrentBrightness;
          TopperALB.RequestSettingValue(SET_TOPPER_BRIGHTNESS, currentAdjustmentByte);
          ALBCurrentBrightness = ConvertALBrightnessToAdjustmentLevel(ALBCurrentBrightness);
          break;
        case OM_BASIC_ADJ_IDS_SPEAKER_BRIGHTNESS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          ALBCurrentBrightness = 0;
          currentAdjustmentByte = &ALBCurrentBrightness;
          TopperALB.RequestSettingValue(SET_SPEAKER_BRIGHTNESS, currentAdjustmentByte);
          ALBCurrentBrightness = ConvertALBrightnessToAdjustmentLevel(ALBCurrentBrightness);
          break;
        case OM_BASIC_ADJ_IDS_LEFT_RAMP_BRIGHTNESS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          ALBCurrentBrightness = 0;
          currentAdjustmentByte = &ALBCurrentBrightness;
          TopperALB.RequestSettingValue(SET_GI_BRIGHTNESS_0, currentAdjustmentByte);
          ALBCurrentBrightness = ConvertALBrightnessToAdjustmentLevel(ALBCurrentBrightness);
          break;
        case OM_BASIC_ADJ_IDS_RIGHT_RAMP_BRIGHTNESS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          ALBCurrentBrightness = 0;
          currentAdjustmentByte = &ALBCurrentBrightness;
          TopperALB.RequestSettingValue(SET_GI_BRIGHTNESS_1, currentAdjustmentByte);
          ALBCurrentBrightness = ConvertALBrightnessToAdjustmentLevel(ALBCurrentBrightness);
          break;
        case OM_BASIC_ADJ_IDS_STADIUM_BRIGHTNESS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          ALBCurrentBrightness = 0;
          currentAdjustmentByte = &ALBCurrentBrightness;
          TopperALB.RequestSettingValue(SET_GI_BRIGHTNESS_2, currentAdjustmentByte);
          ALBCurrentBrightness = ConvertALBrightnessToAdjustmentLevel(ALBCurrentBrightness);
          break;
        case OM_BASIC_ADJ_IDS_GI_IN_ORIGINAL:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&GIInOriginal;
          currentAdjustmentStorageByte = EEPROM_GI_IN_ORIGINAL_BYTE;
          break;
#endif          
      }

      Menus.SetParameterControls(   adjustmentType, numAdjustmentValues, adjustmentValues, parameterCallout,
                                    currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL );
    } else if (topLevel==OPERATOR_MENU_GAME_RULES_LEVEL) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_DIFFICULTY, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
      byte *currentAdjustmentByte = &GameRulesSelection;
      byte adjustmentValues[8] = {0};
      adjustmentValues[0] = 0;
      // if one of the below parameters is installed, the "HasParameterChanged" 
      // check below will install Easy / Medium / Hard rules

      switch (subLevel) {
        case 0:
          adjustmentValues[1] = 1;
          break;
        case 1:
          adjustmentValues[1] = 2;
          break;
        case 2:
          adjustmentValues[1] = 3;
          break;
        case 3:
          adjustmentValues[1] = 4;
          break;
      }

      Menus.SetParameterControls(   OPERATOR_MENU_ADJ_TYPE_LIST, 2, adjustmentValues, (short)SOUND_EFFECT_OM_EASY_RULES_INSTRUCTIONS-1,
                                    EEPROM_GAME_RULES_SELECTION, currentAdjustmentByte, NULL );
                  
    } else if (topLevel==OPERATOR_MENU_GAME_ADJ_MENU) {
      Audio.PlaySound((unsigned short)subLevel + SOUND_EFFECT_AP_DT_HURRYUP_TIME, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);

      byte *currentAdjustmentByte = NULL;
      byte currentAdjustmentStorageByte = 0;
      byte adjustmentValues[8] = {0};
      byte numAdjustmentValues = 2;
      byte adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
      short parameterCallout = 0;
      unsigned long *currentAdjustmentUL = NULL;
      
      adjustmentValues[1] = 1;

      switch (subLevel) {
        case OM_GAME_ADJ_DROP_TARGET_HURRYUP_TIME:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 3;
          adjustmentValues[1] = 12;
          currentAdjustmentByte = &TimeToResetDrops;
          currentAdjustmentStorageByte = EEPROM_DT_HURRYUP_TIME_BYTE;
          parameterCallout = SOUND_EFFECT_AP_DT_HURRYUP_TIME;
          break;
        case OM_SINGLE_COMBAT_NUMBER_OF_SECONDS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 6;
          adjustmentValues[0] = 30;
          adjustmentValues[1] = 45;
          adjustmentValues[2] = 60;
          adjustmentValues[3] = 75;
          adjustmentValues[4] = 90;
          adjustmentValues[5] = 120;
          currentAdjustmentByte = &SingleCombatNumSeconds;
          currentAdjustmentStorageByte = EEPROM_SINGLE_COMBAT_NUM_SECONDS_BYTE;
          break;
        case OM_BALL_SAVE_ON_SINGLE_COMBAT:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 10;
          adjustmentValues[2] = 20;
          adjustmentValues[3] = 35;
          adjustmentValues[4] = 40;
          currentAdjustmentByte = &BallSaveOnCombatModes;
          currentAdjustmentStorageByte = EEPROM_SINGLE_COMBAT_BALL_SAVE_BYTE;
          break;
        case OM_GRACE_TIME_ON_DOUBLE_COMBAT:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 15;
          adjustmentValues[2] = 30;
          adjustmentValues[3] = 45;
          adjustmentValues[4] = 60;
          currentAdjustmentByte = &DoubleCombatNumSeconds;
          currentAdjustmentStorageByte = EEPROM_DOUBLE_COMBAT_NUM_SECONDS_BYTE;
          break;
        case OM_GRACE_TIME_ON_TRIPLE_COMBAT:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 15;
          adjustmentValues[2] = 30;
          adjustmentValues[3] = 45;
          adjustmentValues[4] = 60;
          currentAdjustmentByte = &TripleCombatNumSeconds;
          currentAdjustmentStorageByte = EEPROM_TRIPLE_COMBAT_NUM_SECONDS_BYTE;
          break;
        case OM_COMBOS_UNTIL_OMNIA:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 6;
          adjustmentValues[0] = 12;
          adjustmentValues[1] = 14;
          adjustmentValues[2] = 16;
          adjustmentValues[3] = 18;
          adjustmentValues[4] = 20;
          adjustmentValues[5] = 21;
          currentAdjustmentByte = &CombosUntilOmnia;
          currentAdjustmentStorageByte = EEPROM_COMBOS_UNTIL_OMNIA_BYTE;
          break;
        case OM_MAX_EXTRA_BALLS_PER_BALL:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 1;
          adjustmentValues[1] = 3;
          currentAdjustmentByte = &MaxExtraBallsPerBall;
          currentAdjustmentStorageByte = EEPROM_MAX_EXTRA_BALLS_BYTE;
          break;
        case OM_REQUIRE_PORTCULLIS_FOR_LOCKS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 1;
          currentAdjustmentByte = (byte *)&RequirePortcullisForLocks;
          currentAdjustmentStorageByte = EEPROM_REQUIRE_PORTCULLIS_BYTE;
          break;
        case OM_BASE_TIME_FOR_KINGS_CHALLENGES:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 15;
          adjustmentValues[1] = 20;
          adjustmentValues[2] = 25;
          adjustmentValues[3] = 30;
          adjustmentValues[4] = 35;
          currentAdjustmentByte = &KingsChallengeBaseTime;
          currentAdjustmentStorageByte = EEPROM_TIME_FOR_KCS_BYTE;
          break;
        case OM_GRACE_TIME_FOR_COMBOS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 20;
          adjustmentValues[1] = 25;
          adjustmentValues[2] = 30;
          adjustmentValues[3] = 35;
          adjustmentValues[4] = 40;
          currentAdjustmentByte = &BaseComboTime;
          currentAdjustmentStorageByte = EEPROM_GRACE_TIME_FOR_COMBOS_BYTE;
          break;
        case OM_MAGNASAVE_MAX_SECONDS:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_MIN_MAX;
          adjustmentValues[0] = 0;
          adjustmentValues[1] = 8;
          currentAdjustmentByte = &MagnaSaveMaxSeconds;
          currentAdjustmentStorageByte = EEPROM_MAGNA_SAVE_MAX_BYTE;
          break;
        case OM_SPINS_FOR_BONUS_X:
          adjustmentType = OPERATOR_MENU_ADJ_TYPE_LIST;
          numAdjustmentValues = 5;
          adjustmentValues[0] = 20;
          adjustmentValues[1] = 25;
          adjustmentValues[2] = 30;
          adjustmentValues[3] = 35;
          adjustmentValues[4] = 40;
          currentAdjustmentByte = &BaseSpinsUntilSpinnerGoal;
          currentAdjustmentStorageByte = EEPROM_SPINS_UNTIL_GOAL_BYTE;
          break;
      }
      
      Menus.SetParameterControls(   adjustmentType, numAdjustmentValues, adjustmentValues, parameterCallout,
                                    currentAdjustmentStorageByte, currentAdjustmentByte, currentAdjustmentUL );
    }    
  }

  if (Menus.HasParameterChanged()) {
    short parameterCallout = Menus.GetParameterCallout();
    if (parameterCallout) {
      Audio.StopAllAudio();
      Audio.PlaySound((unsigned short)parameterCallout + Menus.GetParameterID(), AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
    }
    if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_RULES_LEVEL) {
      // Install the new rules level
      if (LoadRuleDefaults(GameRulesSelection)) {
        WriteParameters();
      }
    } else if (Menus.GetTopLevel()==OPERATOR_MENU_BASIC_ADJ_MENU) {
      if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_MUSIC_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_BACKGROUND_SONG_1, AUDIO_PLAY_TYPE_WAV_TRIGGER, MusicVolume);
        Audio.SetMusicVolume(MusicVolume);
        SoundSettingTimeout = CurrentTime + 5000;
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_SOUNDFX_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_DROP_TARGET_COMPLETE_2, AUDIO_PLAY_TYPE_WAV_TRIGGER, SoundEffectsVolume);
        Audio.SetSoundFXVolume(SoundEffectsVolume);
        SoundSettingTimeout = CurrentTime + 5000;
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_CALLOUTS_VOLUME) {
        if (SoundSettingTimeout) Audio.StopAllAudio();
        Audio.PlaySound(SOUND_EFFECT_VP_JACKPOT, AUDIO_PLAY_TYPE_WAV_TRIGGER, CalloutsVolume);
        Audio.SetNotificationsVolume(CalloutsVolume);
        SoundSettingTimeout = CurrentTime + 3000; 
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_TOPPER_BRIGHTNESS) {
        TopperALB.AdjustSetting(SET_TOPPER_BRIGHTNESS, ConvertAdjustmentLevelToALBBrightness(ALBCurrentBrightness), 0, 0);
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 255, 255, 255);
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_SPEAKER_BRIGHTNESS) {
        TopperALB.AdjustSetting(SET_SPEAKER_BRIGHTNESS, ConvertAdjustmentLevelToALBBrightness(ALBCurrentBrightness), 0, 0);
        TopperALB.LoopAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 255, 255, 255);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 255, 255, 255);
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_LEFT_RAMP_BRIGHTNESS) {
        TopperALB.StopAnimation();
        TopperALB.AdjustSetting(SET_GI_BRIGHTNESS_0, ConvertAdjustmentLevelToALBBrightness(ALBCurrentBrightness), 0, 0);
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_RIGHT_RAMP_BRIGHTNESS) {
        TopperALB.StopAnimation();
        TopperALB.AdjustSetting(SET_GI_BRIGHTNESS_1, ConvertAdjustmentLevelToALBBrightness(ALBCurrentBrightness), 0, 0);
      } else if (Menus.GetSubLevel()==OM_BASIC_ADJ_IDS_STADIUM_BRIGHTNESS) {
        TopperALB.StopAnimation();
        TopperALB.AdjustSetting(SET_GI_BRIGHTNESS_2, ConvertAdjustmentLevelToALBBrightness(ALBCurrentBrightness), 0, 0);
      }
#else
      }
#endif      
    } else if (Menus.GetTopLevel()==OPERATOR_MENU_GAME_ADJ_MENU) {
    }
  }

  if (SoundSettingTimeout && CurrentTime>SoundSettingTimeout) {
    SoundSettingTimeout = 0;
    Audio.StopAllAudio();
  }

  if (SoundTestStart && CurrentTime>SoundTestStart) {
    if (SoundTestSequence==0) {
      PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1);
      SoundTestSequence = 1;
    } else if (SoundTestSequence==1 && CurrentTime>(SoundTestStart+5000)) {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_COMPLETE_2);
      SoundTestSequence = 2;
    } else if (SoundTestSequence==2 && CurrentTime>(SoundTestStart+10000)) {
      Audio.QueuePrioritizedNotification(SOUND_EFFECT_VP_JACKPOT, 0, 10, CurrentTime);
      SoundTestSequence = 3;
    }
  }
  
}





////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////
void PlayBackgroundSong(unsigned int songNum) {

  if (MusicVolume == 0) return;

  Audio.PlayBackgroundSong(songNum);
}


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned int soundEffectNum) {

  if (MachineState == MACHINE_STATE_INIT_GAMEPLAY) return;
  Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  /*
    switch (soundEffectNum) {
      case SOUND_EFFECT_LEFT_SHOOTER_LANE:
        Audio.PlaySoundCardWhenPossible(12, CurrentTime, 0, 500, 7);
        break;
      case SOUND_EFFECT_RETURN_TO_SHOOTER_LANE:
        Audio.PlaySoundCardWhenPossible(22, CurrentTime, 0, 500, 8);
        break;
      case SOUND_EFFECT_SAUCER:
        Audio.PlaySoundCardWhenPossible(14, CurrentTime, 0, 500, 7);
        break;
      case SOUND_EFFECT_DROP_TARGET_HURRY:
        Audio.PlaySoundCardWhenPossible(2, CurrentTime, 0, 45, 3);
        break;
      case SOUND_EFFECT_DROP_TARGET_COMPLETE:
        Audio.PlaySoundCardWhenPossible(9, CurrentTime, 0, 1400, 4);
        Audio.PlaySoundCardWhenPossible(19, CurrentTime, 1500, 10, 4);
        break;
      case SOUND_EFFECT_HOOFBEATS:
        Audio.PlaySoundCardWhenPossible(12, CurrentTime, 0, 100, 10);
        break;
      case SOUND_EFFECT_STOP_BACKGROUND:
        Audio.PlaySoundCardWhenPossible(19, CurrentTime, 0, 10, 10);
        break;
      case SOUND_EFFECT_DROP_TARGET_HIT:
        Audio.PlaySoundCardWhenPossible(7, CurrentTime, 0, 150, 5);
        break;
      case SOUND_EFFECT_SPINNER:
        Audio.PlaySoundCardWhenPossible(6, CurrentTime, 0, 25, 2);
        break;
    }
  */
}


void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume == 0) return;
  if (SoundSelector < 3 || SoundSelector == 4 || SoundSelector == 7 || SoundSelector == 9) return;
  //if (soundEffectNum < SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum >= (SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START + NUM_VOICE_NOTIFICATIONS)) return;

  Audio.QueuePrioritizedNotification(soundEffectNum, 0, priority, CurrentTime);
}


void AlertPlayerUp(byte playerNum) {
  //  (void)playerNum;
  //  QueueNotification(SOUND_EFFECT_VP_PLAYER, 1);
  QueueNotification(SOUND_EFFECT_VP_PLAYER_ONE_UP + playerNum, 1);
  //  QueueNotification(SOUND_EFFECT_VP_LAUNCH_WHEN_READY, 1);
}





////////////////////////////////////////////////////////////////////////////
//
//  Diagnostics Mode
//
////////////////////////////////////////////////////////////////////////////

int RunDiagnosticsMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {

    /*
        char buf[256];
        boolean errorSeen;

        Serial.write("Testing Volatile RAM at IC13 (0x0000 - 0x0080): writing & reading... ");
        Serial.write("3 ");
        delay(500);
        Serial.write("2 ");
        delay(500);
        Serial.write("1 \n");
        delay(500);
        errorSeen = false;
        for (byte valueCount=0; valueCount<0xFF; valueCount++) {
          for (unsigned short address=0x0000; address<0x0080; address++) {
            RPU_DataWrite(address, valueCount);
          }
          for (unsigned short address=0x0000; address<0x0080; address++) {
            byte readValue = RPU_DataRead(address);
            if (readValue!=valueCount) {
              sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, readValue);
              Serial.write(buf);
              errorSeen = true;
            }
            if (errorSeen) break;
          }
          if (errorSeen) break;
        }
        if (errorSeen) {
          Serial.write("!!! Error in Volatile RAM\n");
        }

        Serial.write("Testing Volatile RAM at IC16 (0x0080 - 0x0100): writing & reading... ");
        Serial.write("3 ");
        delay(500);
        Serial.write("2 ");
        delay(500);
        Serial.write("1 \n");
        delay(500);
        errorSeen = false;
        for (byte valueCount=0; valueCount<0xFF; valueCount++) {
          for (unsigned short address=0x0080; address<0x0100; address++) {
            RPU_DataWrite(address, valueCount);
          }
          for (unsigned short address=0x0080; address<0x0100; address++) {
            byte readValue = RPU_DataRead(address);
            if (readValue!=valueCount) {
              sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, readValue);
              Serial.write(buf);
              errorSeen = true;
            }
            if (errorSeen) break;
          }
          if (errorSeen) break;
        }
        if (errorSeen) {
          Serial.write("!!! Error in Volatile RAM\n");
        }

        // Check the CMOS RAM to see if it's operating correctly
        errorSeen = false;
        Serial.write("Testing CMOS RAM: writing & reading... ");
        Serial.write("3 ");
        delay(500);
        Serial.write("2 ");
        delay(500);
        Serial.write("1 \n");
        delay(500);
        for (byte valueCount=0; valueCount<0x10; valueCount++) {
          for (unsigned short address=0x0100; address<0x0200; address++) {
            RPU_DataWrite(address, valueCount);
          }
          for (unsigned short address=0x0100; address<0x0200; address++) {
            byte readValue = RPU_DataRead(address);
            if ((readValue&0x0F)!=valueCount) {
              sprintf(buf, "Write/Read failure at address=0x%04X (expected 0x%02X, read 0x%02X)\n", address, valueCount, (readValue&0x0F));
              Serial.write(buf);
              errorSeen = true;
            }
            if (errorSeen) break;
          }
          if (errorSeen) break;
        }

        if (errorSeen) {
          Serial.write("!!! Error in CMOS RAM\n");
        }


        // Check the ROMs
        Serial.write("CMOS RAM dump... ");
        Serial.write("3 ");
        delay(500);
        Serial.write("2 ");
        delay(500);
        Serial.write("1 \n");
        delay(500);
        for (unsigned short address=0x0100; address<0x0200; address++) {
          if ((address&0x000F)==0x0000) {
            sprintf(buf, "0x%04X:  ", address);
            Serial.write(buf);
          }
      //      RPU_DataWrite(address, address&0xFF);
          sprintf(buf, "0x%02X ", RPU_DataRead(address));
          Serial.write(buf);
          if ((address&0x000F)==0x000F) {
            Serial.write("\n");
          }
        }

    */

    //    RPU_EnableSolenoidStack();
    //    RPU_SetDisableFlippers(false);

  }

  return returnState;
}

////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////
unsigned long AttractModeStartTime = 0;
unsigned long AttractModeSpecialSequenceStart = 0;
byte AttractModeSpecialSequenceStage = 0;
byte AttractModeStage = 0;
byte AttractModeLastHeadStage = 0;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    AttractModeStartTime = CurrentTime;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.StopAnimation();
    TopperALB.PlayAnimation(GI_0_SET_COLOR, 250, 0, 0);
    TopperALB.PlayAnimation(GI_1_SET_COLOR, 150, 27, 0);
#endif
    ResetGIHolds();
    SetGeneralIlluminationOn(true);
    RPU_SetCoinLockout((FreePlayMode || Credits >= MaximumCredits) ? true : false, SOLCONT_COIN_LOCKOUT);
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }

    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    Display_ClearOverride(0xFF);
    Display_UpdateDisplays(0xFF);
    RPU_SetLampState(LAMP_APRON_CREDITS, (Credits || FreePlayMode));

    // If the machine has just been powered on, 
    // we want to show the version in the displays
    // and start the special attract sequence
    if (CurrentTime<10000) {
      Display_OverrideScoreDisplay(0, BK_MAJOR_VERSION, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      Display_OverrideScoreDisplay(1, BK_MINOR_VERSION, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      Display_OverrideScoreDisplay(2, RPU_OS_MAJOR_VERSION, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      Display_OverrideScoreDisplay(3, RPU_OS_MINOR_VERSION, DISPLAY_OVERRIDE_ANIMATION_CENTER);
      AttractModeSpecialSequenceStart = CurrentTime + 5000;
      AttractModeSpecialSequenceStage = 0;
      Display_UpdateDisplays(); 
    } else {
      AttractModeSpecialSequenceStart = CurrentTime + 300000;
      AttractModeSpecialSequenceStage = 0;
    }
    
    // Update MachineLocks, and kick the ball from the saucer
    // (if it's there). MachineLocks doesn't include the saucer
    // unless we're playing a game.
    MachineLocks = InitializeMachineLocksBasedOnSwitches();
    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      if (SaucerEjectTime == 0 || CurrentTime > (SaucerEjectTime + 2500)) {
        RPU_PushToSolenoidStack(SOL_SAUCER, 12, true);
        SaucerEjectTime = CurrentTime;
      }
    }
  }

  UpdateLockStatus();
  MoveBallFromOutholeToRamp();

  if (AttractModeSpecialSequenceStart!=0 && CurrentTime>AttractModeSpecialSequenceStart) {
    if (AttractModeSpecialSequenceStage==0) {
      RPU_TurnOffAllLamps();
      SetGeneralIlluminationOn(false);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 140, 0, 200);
      TopperALB.LoopAnimation(GI_0_PULSE_COLOR, 140, 0, 200);
      TopperALB.LoopAnimation(GI_1_PULSE_COLOR, 140, 0, 200);
      TopperALB.LoopAnimation(GI_2_PULSE_COLOR, 140, 0, 200);
#endif
      AttractModeSpecialSequenceStage = 1;
    } else if (AttractModeSpecialSequenceStage==1 && CurrentTime>(AttractModeSpecialSequenceStart+2000)) {
      AttractModeSpecialSequenceStage = 2;
      AttractModeSpecialSequenceStart = CurrentTime - 1;
      PlaySoundEffect(SOUND_EFFECT_MACHINE_START_1);
    } else if (AttractModeSpecialSequenceStage==2) {
      byte lampsPhase = (CurrentTime-(AttractModeSpecialSequenceStart))/150;
      if (lampsPhase<LAMP_ANIMATION_STEPS) {  
        FlashAnimationSteps(1, lampsPhase, 50);
      } else {
        AttractModeSpecialSequenceStage = 3;
        AttractModeSpecialSequenceStart = CurrentTime - 1;
        RPU_TurnOffAllLamps();
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 140, 40, 0);
        TopperALB.AdjustSetting(SET_COLOR_0, 200, 0, 150);
        TopperALB.AdjustSetting(SET_COLOR_1, 140, 40, 0);
        TopperALB.LoopAnimation(BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE);
        TopperALB.LoopAnimation(GI_0_PULSE_COLOR, 140, 40, 0);
        TopperALB.LoopAnimation(GI_1_PULSE_COLOR, 140, 40, 0);
        TopperALB.LoopAnimation(GI_2_PULSE_COLOR, 140, 40, 0);
#endif
      }
    } else if (AttractModeSpecialSequenceStage==3) {
      byte lampsPhase = (CurrentTime-(AttractModeSpecialSequenceStart))/200;
      if (lampsPhase<LAMP_ANIMATION_STEPS) {  
        FlashAnimationSteps(2, lampsPhase, 50);
      } else {
        AttractModeSpecialSequenceStage = 4;
        RPU_TurnOffAllLamps();
        AttractModeSpecialSequenceStart = CurrentTime - 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
#endif
      }
    } else if (AttractModeSpecialSequenceStage==4 && CurrentTime>(AttractModeSpecialSequenceStart+1000)) {
      AttractModeSpecialSequenceStage = 99;
      Audio.PlaySoundCardWhenPossible(20 + (CurrentTime%3), CurrentTime, 0, 1000, 10);
    } else if (AttractModeSpecialSequenceStage==99) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.StopAnimation();
#endif
      RPU_TurnOffAllLamps();
      AttractModeSpecialSequenceStage = 0;
      AttractModeSpecialSequenceStart = CurrentTime + 600000;
    }
  } else {
    SetGeneralIlluminationOn(true);
    AttractModeStage = ((CurrentTime - AttractModeStartTime) / 1000)%24;
    byte lampPhase = ((CurrentTime - AttractModeStartTime)/100)%LAMP_ANIMATION_STEPS;
    if (AttractModeStage<6) {      
      ShowLampAnimation(0, 30, CurrentTime, 14, false, false);
    } else if (AttractModeStage<12) {
      ShowLampAnimationSingleStep(3, lampPhase);
    } else if (AttractModeStage<18) {
      ShowLampAnimation(1, 30, CurrentTime, 18, false, false);
    } else {
      ShowLampAnimation(3, 15, CurrentTime, 12, false, true);
    }
  }


  // Update the head with lamps and scores
  byte headStage = (CurrentTime - AttractModeStartTime) / 1000;
  if (headStage<16) {
    AttractModeLastHeadStage = 0xFF;
    RPU_SetLampState(LAMP_HEAD_HIGH_SCORE_TO_DATE, 0);
    RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1, 0, 500);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);
    RPU_SetLampState(LAMP_HEAD_BALL_IN_PLAY, 0);
    RPU_SetLampState(LAMP_HEAD_MATCH, 0);
    RPU_SetLampState(LAMP_HEAD_TIMER_BONUS_BALL, 0);

    if (CurrentTime<30000) {

    } else {
      // update to scroll scores of last game
      Display_UpdateDisplays();
    }
  } else {
    if (headStage!=AttractModeLastHeadStage) {
      AttractModeLastHeadStage = headStage;
      if ( ((headStage/8)%2) ) {
        RPU_SetLampState(LAMP_HEAD_HIGH_SCORE_TO_DATE, 1, 0, 250);
        RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
        RPU_SetLampState(LAMP_HEAD_TILT, 0);
        RPU_SetLampState(LAMP_HEAD_BALL_IN_PLAY, 0);
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        RPU_SetLampState(LAMP_HEAD_TIMER_BONUS_BALL, 0);
        Display_SetLastTimeScoreChanged(CurrentTime);
        Display_ClearOverride(0xFF);
      } else {
        RPU_SetLampState(LAMP_HEAD_HIGH_SCORE_TO_DATE, 0);
        RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
        RPU_SetLampState(LAMP_HEAD_TILT, 0);
        RPU_SetLampState(LAMP_HEAD_BALL_IN_PLAY, 0);
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        RPU_SetLampState(LAMP_HEAD_TIMER_BONUS_BALL, 0);
        Display_SetLastTimeScoreChanged(CurrentTime);
        Display_ClearOverride(0xFF);
      }
    }

    // Update every loop to show scrolling scores when necessary
    if ( (headStage/8)%2 ) {
      Display_UpdateDisplays(0xFF, false, false, false, HighScore);
    } else {
      Display_UpdateDisplays();
    }
  }
  
/*
  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      Display_ClearOverride(0xFF);
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetDisplayBallInPlay(0, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE_TO_DATE, 1, 0, 250);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      Display_SetLastTimeScoreChanged(CurrentTime);
    }
    AttractLastHeadMode = 2;
    Display_UpdateDisplays(0xFF, false, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE_TO_DATE, 0);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      Display_SetLastTimeScoreChanged(CurrentTime);
    }
    Display_ClearOverride(0xFF);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 4);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    RPU_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
//    AttractLastLadderBonus = 1;
//    AttractLastLadderTime = CurrentTime;
  }

  ShowLampAnimation(attractPlayfieldPhase, 30, CurrentTime, 14, false, false);
*/

  byte switchHit;
  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH /*&& (CurrentTime - GetLastSelfTestChangedTime()) > 250*/) {
      Menus.EnterOperatorMenu();
    }
  }

  // If the user was holding the menu button when the game started
  // then kick the balls
  if (CurrentTime < 4000) {
    if (RPU_ReadSingleSwitchState(SW_SELF_TEST_SWITCH)) {
      if (OperatorSwitchPressStarted==0) {
        OperatorSwitchPressStarted = CurrentTime;
      } else if (CurrentTime > (OperatorSwitchPressStarted+500)) {
        Menus.EnterOperatorMenu();
        Menus.BallEjectInProgress(true);
      }
    } else {
      OperatorSwitchPressStarted = 0;
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(unsigned short intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode;
  GameModeStartTime = 0;
  GameModeEndTime = 0;
}

byte CountBallsInTrough() {
  // RPU_ReadSingleSwitchState(SW_OUTHOLE) +

  byte numBalls = RPU_ReadSingleSwitchState(SW_LEFT_BALL_RAMP) +
                  RPU_ReadSingleSwitchState(SW_CENTER_BALL_RAMP) +
                  RPU_ReadSingleSwitchState(SW_RIGHT_BALL_RAMP);

  return numBalls;
}



void AddToBonus(byte bonus) {
  Bonus[CurrentPlayer] += bonus;
  if (Bonus[CurrentPlayer] > MAX_DISPLAY_BONUS) {
    Bonus[CurrentPlayer] = MAX_DISPLAY_BONUS;
  } else {
    BonusAnimationStart = CurrentTime;
  }
}



void IncreaseBonusX() {

  if (BonusX[CurrentPlayer] < 10) {
    BonusX[CurrentPlayer] += 1;
    BonusXAnimationStart = CurrentTime;
    PlaySoundEffect(SOUND_EFFECT_BONUS_2X_AWARD + (BonusX[CurrentPlayer] - 2));
  }

}

unsigned long GameStartNotificationTime = 0;
boolean WaitForBallToReachOuthole = false;
unsigned long UpperBallEjectTime = 0;

int InitGamePlay(boolean curStateChanged) {
  RPU_TurnOffAllLamps();
  RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
  SetGeneralIlluminationOn(true);

  if (curStateChanged) {

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 50);
    TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 50);
#endif
    
    GameStartNotificationTime = CurrentTime;

    // Reset displays & game state variables
    for (int count = 0; count < 4; count++) {
      // Initialize game-specific variables
      BonusX[count] = 1;
      Bonus[count] = 1;
      SpinsTowardsNextGoal[count] = 0;
      SpinnerGoal[count] = BaseSpinsUntilSpinnerGoal;
      PlayerLockStatus[count] = 0;
      CombosAchieved[count] = 0;
      RPU_SetDisplayBlank(count, 0x00);
      CurrentScores[count] = 0;
      RightRampValue[count] = 0;
      MagnaStatusLeft[count] = 0;
      MagnaStatusRight[count] = 0;
      SingleCombatLevelCompleted[count] = 0;
      DoubleCombatLevelCompleted[count] = 0;
      TripleCombatLevelCompleted[count] = 0;
      CombatJackpot[count] = 0; 
      DropTargetLevel[count] = 0;
      ExtraBallsOrSpecialAvailable[count] = 0;
      KingsChallengeStatus[count] = 0;
      SkillShotsMade[count] = 0;

      LastChanceStatus[count] = 0;
      for (int i = 0; i < 4; i++) {
        NumDropTargetHits[count][i] = 0;
        NumDropTargetClears[count][i] = 0;
      }
    }

    SamePlayerShootsAgain = false;
    CurrentBallInPlay = 1;
    CurrentNumPlayers = 1;
    CurrentPlayer = 0;
  }

  if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
    if (SaucerEjectTime == 0 || CurrentTime > (SaucerEjectTime + 2500)) {
      RPU_PushToSolenoidStack(SOL_SAUCER, 12, true);
      SaucerEjectTime = CurrentTime;
    }
  }

  NumberOfBallsInPlay = 0;
  UpdateLockStatus();
  NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);

  if (NumberOfBallsLocked == TotalBallsLoaded) {
    // we have to kick a ball
    ReleaseLockedBall();
  }

  if (CountBallsInTrough() < (TotalBallsLoaded - NumberOfBallsLocked) /*|| RPU_ReadSingleSwitchState(SW_SHOOTER_LANE) */) {

    if (!RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
      MoveBallFromOutholeToRamp();
  
      if (CurrentTime > (GameStartNotificationTime + 5000)) {
        GameStartNotificationTime = CurrentTime;
        QueueNotification(SOUND_EFFECT_VP_BALL_MISSING, 10);
      }
  
      return MACHINE_STATE_INIT_GAMEPLAY;
    }
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  RPU_EnableSolenoidStack();
  RPU_SetCoinLockout((FreePlayMode || Credits >= MaximumCredits) ? true : false, SOLCONT_COIN_LOCKOUT);

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {

    if (GameRulesSelection==GAME_RULES_PROGRESSIVE) {
      // if the operator has installed "progressive" rules, 
      // then we change the rules based on player number.
      // If we're 1 player, it's EASY
      // First player is EASY, last player is HARD,
      // and in-between players are MEDIUM
      byte rulesLevel = GAME_RULES_EASY;
      if (CurrentPlayer==0) rulesLevel = GAME_RULES_EASY;
      else if (CurrentPlayer==(CurrentNumPlayers-1)) rulesLevel = GAME_RULES_HARD;
      else rulesLevel = GAME_RULES_MEDIUM;
      LoadRuleDefaults(rulesLevel);
      if (CurrentBallInPlay==1) QueueNotification(SOUND_EFFECT_VP_GAME_RULES_EASY + (rulesLevel-GAME_RULES_EASY), 10);
    }

    RPU_TurnOffAllLamps();
    RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
    BallFirstSwitchHitTime = 0;

    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (CurrentBallInPlay != 1 || CurrentPlayer != 0) && !SamePlayerShootsAgain) AlertPlayerUp(CurrentPlayer);
    SamePlayerShootsAgain = false;

    RPU_SetDisplayBallInPlay(CurrentBallInPlay);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);

    if (BallSaveNumSeconds > 0) {
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
      RPU_SetLampState(LAMP_HEAD_SHOOT_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    PlayLockKickWarningTime = 0;

    GIReturnState = true;
    for (byte count=0; count<GI_OVERRIDE_NUMBER_OF_HOLDS; count++) {
      GIOverrideOffTime[count] = 0;
      GIOverrideHoldTime[count] = 0;
    }
    SetGeneralIlluminationOn();

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    for (byte count = 0; count < 4; count++) {
      DropTargetResetTime[count] = 0;
      DropTargetHurryTime[count] = 0;
      DropTargetHurryLamp[count] = false;
    }

    ExtraBallsCollected = 0;
    SpecialCollected = false;

    // If not held over, reset bonus & bonusx
    if (CombosAchieved[CurrentPlayer] & COMBO_RIGHT_INLANE_SPINNER) {
      if (BonusX[CurrentPlayer]==1) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal;
      else if (BonusX[CurrentPlayer]==2) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 10;
      else if (BonusX[CurrentPlayer]==3) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 20;
      else if (BonusX[CurrentPlayer]==4) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 30;
      else if (BonusX[CurrentPlayer]==5) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 40;
      else if (BonusX[CurrentPlayer]==6) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 50;
      else if (BonusX[CurrentPlayer]==7) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 60;
      else if (BonusX[CurrentPlayer]==8) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 70;
      else if (BonusX[CurrentPlayer]==9) SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal + 80;
    } else {
      BonusX[CurrentPlayer] = 1;
      SpinnerGoal[CurrentPlayer] = BaseSpinsUntilSpinnerGoal;
    }

    if (CombosAchieved[CurrentPlayer] & COMBO_RIGHT_INLANE_LOOP) {
      // Bonus isn't reset      
    } else {
      Bonus[CurrentPlayer] = 1;
    }

    if (CombosAchieved[CurrentPlayer] & COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP_LOCK) {
      // Double timers
      DoubleTimers = true;
    } else {
      DoubleTimers = false;
    }

    if (CombosAchieved[CurrentPlayer] & COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP) {
      // RightRampValue is not reset
    } else {      
      RightRampValue[CurrentPlayer] = 0;
    }

    if (CurrentBallInPlay==BallsPerGame) {
      LastChanceAvailable = true;
    } else {
      LastChanceAvailable = false;
    }
    
    BonusXCollectAvailable = false;
    SpinsTowardsNextGoal[CurrentPlayer] = 0;
    BonusXCollectAvailableStart = 0;
    BonusXCollectReminder = 0;
    NumBonusXCollectReminders = 0;
    ExtraBallAwardStartTime = 0;
    KingsChallengeEndTime = 0;
    KingsChallengeRunning = 0;
    KingsChallengeKick = 0;
    LevitateMagnetOffTime = 0;
    LevitateMagnetOnTime = 0;
    LastTimeLeftMagnetOn = 0;
    LastTimeRightMagnetOn = 0;
    KingsChallengePerfectionBank = 0;
    KCBeginPlayed = false;

    PlayfieldMultiplier = 1;
    PlayfieldMultiplierExpiration = 0;
    Display_ResetDisplayTrackingVariables();
    BonusXAnimationStart = 0;
    LastPopBumperHit = 0;
    LastSpinnerHit = 0;
    SpinnerLitUntil = 0;
    SpinnerStatus = 0;
    LoopLitToQualifyLock = false;
    BonusAnimationStart = 0;
    MagnaSaveAvailable = false;
    MagnaSoundOn = false;
    LockManagementInProgress = false;
    LastLoopHitTime = CurrentTime;
    TripleCombatJackpotsAvailable = 0;

    for (byte count = 0; count < 3; count++) {
      LockKickTime[count] = 0;
    }
    SaucerKickTime = 0;
    PlaySaucerKickWarningTime = 0;

    BallSaveEndTime = 0;
    IdleMode = IDLE_MODE_NONE;

    LastLeftInlane = 0;
    LastRightInlane = 0;
    LastLeftOutlane = 0;
    LastRightOutlane = 0;

    LeftComboLastHitTime = 0;
    RightComboLastHitTime = 0;
    ComboStep = 0;

    SpinnerLitFromCombo = false;
    RightRampLitFromCombo = false;
    SaucerLitFromCombo = false;
    LockLitFromCombo = false;
    LoopLitFromCombo = false;
    LeftInlaneLitFromCombo = false;
    RightInlaneLitFromCombo = false;
    UpperLeftRolloverLitFromCombo = false;
    

//    for (byte count = 0; count < NUM_BALL_SEARCH_SOLENOIDS; count++) {
//      BallSearchSolenoidFireTime[count] = 0;
//    }

    if (CurrentPlayer == 0) {
      // Only change skill shot on first ball of round.
      SkillShotTarget = CurrentTime % 3;
    }

    // Reset Drop Targets
    DropTargetsUL.ResetDropTargets(CurrentTime + 100, true);
    DropTargetsUR.ResetDropTargets(CurrentTime + 250, true);
    DropTargetsLL.ResetDropTargets(CurrentTime + 400, true);
    DropTargetsLR.ResetDropTargets(CurrentTime + 550, true);

    if (!RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) RPU_PushToTimedSolenoidStack(SOL_BALL_RAMP_THROWER, 16, CurrentTime + 1000);
    NumberOfBallsInPlay = 1;
    LastTimeBallServed = CurrentTime + 1000;

    // See if any locks have been stolen and move them from locked to availble
    UpdatePlayerLocks();
    byte rallyNum = CurrentBallInPlay - 1;
    if (BallsPerGame > 3) rallyNum = (CurrentBallInPlay > 1) ? ((CurrentBallInPlay == 5) ? 2 : 1) : 0;
    PlayBackgroundSong(SOUND_EFFECT_RALLY_SONG_1 + rallyNum);
  }

  //  if (CountBallsInTrough()==(TotalBallsLoaded-NumberOfBallsLocked)) {

  byte ballInShooter = RPU_ReadSingleSwitchState(SW_SHOOTER_LANE) ? 1 : 0;
  // We'll wait until we see the ball in the shooter lane
  if (!ballInShooter) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

  LastTimeThroughLoop = CurrentTime;
}





void AnnounceStatus() {
  /*
        if (TicksCountedTowardsStatus > 68000) {
          IdleMode = IDLE_MODE_NONE;
          TicksCountedTowardsStatus = 0;
        } else if (TicksCountedTowardsStatus > 59000) {
          if (IdleMode != IDLE_MODE_BALL_SEARCH) {
            BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime - 1;
          }
          if (CurrentTime > BallSearchNextSolenoidTime) {
            // Fire off a solenoid
            BallSearchSolenoidFireTime[BallSearchSolenoidToTry] = CurrentTime;
            RPU_PushToSolenoidStack(BallSearchSols[BallSearchSolenoidToTry], 10);
            BallSearchSolenoidToTry += 1;
            if (BallSearchSolenoidToTry >= NUM_BALL_SEARCH_SOLENOIDS) BallSearchSolenoidToTry = 0;
            BallSearchNextSolenoidTime = CurrentTime + 500;
          }
          IdleMode = IDLE_MODE_BALL_SEARCH;
        } else if (TicksCountedTowardsStatus > 52000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SHIELD) {
            TicksCountedTowardsStatus = 59001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SHIELD) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SHIELD, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SHIELD;
          }
        } else if (TicksCountedTowardsStatus > 45000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_SPINS) {
            TicksCountedTowardsStatus = 52001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_SPINS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_SPINS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_SPINS;
          }
        } else if (TicksCountedTowardsStatus > 38000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_7_NZ) {
            TicksCountedTowardsStatus = 45001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_NZS) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_NZS, 1);
            IdleMode = IDLE_MODE_ADVERTISE_NZS;
            ShowLampAnimation(0, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        } else if (TicksCountedTowardsStatus > 31000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_POP_BASES) {
            TicksCountedTowardsStatus = 38001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BASES) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BASES, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BASES;
          }
        } else if (TicksCountedTowardsStatus > 24000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_COMBOS) {
            TicksCountedTowardsStatus = 31001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_COMBOS) {
              byte countBits = CountBits(CombosAchieved[CurrentPlayer]);
              if (countBits==0) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_COMBOS, 1);
              else if (countBits>0) QueueNotification(SOUND_EFFECT_VP_FIVE_COMBOS_LEFT+(countBits-1), 1);
            }
            IdleMode = IDLE_MODE_ADVERTISE_COMBOS;
          }
        } else if (TicksCountedTowardsStatus > 17000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_INVASION) {
            TicksCountedTowardsStatus = 24001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_INVASION) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_INVASION, 1);
            IdleMode = IDLE_MODE_ADVERTISE_INVASION;
          }
        } else if (TicksCountedTowardsStatus > 10000) {
          if (WizardGoals[CurrentPlayer]&WIZARD_GOAL_BATTLE) {
            TicksCountedTowardsStatus = 17001;
          } else {
            if (IdleMode != IDLE_MODE_ADVERTISE_BATTLE) QueueNotification(SOUND_EFFECT_VP_ADVERTISE_BATTLE, 1);
            IdleMode = IDLE_MODE_ADVERTISE_BATTLE;
          }
        } else if (TicksCountedTowardsStatus > 7000) {
          int goalCount = (int)(CountBits((WizardGoals[CurrentPlayer] & ~UsedWizardGoals[CurrentPlayer]))) + NumCarryWizardGoals[CurrentPlayer];
          if (GoalsUntilWizard==0) {
            TicksCountedTowardsStatus = 10001;
          } else {
            byte goalsRemaining = GoalsUntilWizard-(goalCount%GoalsUntilWizard);
            if (goalCount<0) goalsRemaining = (byte)(-1*goalCount);

            if (IdleMode != IDLE_MODE_ANNOUNCE_GOALS) {
              QueueNotification(SOUND_EFFECT_VP_ONE_GOAL_FOR_ENEMY-(goalsRemaining-1), 1);
              if (DEBUG_MESSAGES) {
                char buf[256];
                sprintf(buf, "Goals remaining = %d, Goals Until Wiz = %d, goalcount = %d, LO=%d, WizG=0x%04X\n", goalsRemaining, GoalsUntilWizard, goalCount, WizardGoals[CurrentPlayer], NumCarryWizardGoals[CurrentPlayer]);
                Serial.write(buf);
              }
            }
            IdleMode = IDLE_MODE_ANNOUNCE_GOALS;
            ShowLampAnimation(2, 40, CurrentTime, 11, false, false);
            specialAnimationRunning = true;
          }
        }
  */
}


boolean AddABall(boolean ballLocked = false, boolean ballSave = true) {
  if (NumberOfBallsInPlay >= TotalBallsLoaded) return false;

  if (ballLocked) {
    NumberOfBallsLocked += 1;
  } else {
    NumberOfBallsInPlay += 1;
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Num BIP +1 to %d b/c AddABall\n", NumberOfBallsInPlay);
      Serial.write(buf);
    }
  }

  if (CountBallsInTrough()) {
    RPU_PushToTimedSolenoidStack(SOL_BALL_RAMP_THROWER, 16, CurrentTime + 100);
  } else {
    if (ballLocked) {
      if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Num BIP minus 1 to %d b/c AddABall\n", NumberOfBallsInPlay);
        Serial.write(buf);
      }
    } else {
      return false;
    }
  }

  if (ballSave) {
    if (BallSaveEndTime) BallSaveEndTime += 10000;
    else BallSaveEndTime = CurrentTime + 20000;
  }

  return true;
}


void UpdateDropTargets() {
  DropTargetsLL.Update(CurrentTime);
  DropTargetsLR.Update(CurrentTime);
  DropTargetsUL.Update(CurrentTime);
  DropTargetsUR.Update(CurrentTime);

  for (byte count = 0; count < 4; count++) {

    DropTargetBank *curBank;
    if (count == 0) curBank = &DropTargetsUL;
    else if (count == 1) curBank = &DropTargetsUR;
    else if (count == 2) curBank = &DropTargetsLL;
    else if (count == 3) curBank = &DropTargetsLR;

    if (DropTargetResetTime[count] && curBank->GetStatus(false)) {
      if (CurrentTime > (DropTargetResetTime[count])) {
        DropTargetResetTime[count] = 0;
        curBank->ResetDropTargets(CurrentTime);
        if (count == 0) DropTargetsUL.ResetDropTargets(CurrentTime);
        if (count == 1) DropTargetsUR.ResetDropTargets(CurrentTime);
        if (count == 2) DropTargetsLL.ResetDropTargets(CurrentTime);
        if (count == 3) DropTargetsLR.ResetDropTargets(CurrentTime);
      } else {
        // Figure out if hurry lamp should be on (and if sound should play)
        if (CurrentTime > DropTargetHurryTime[count]) {
          unsigned long timeUntilNext = (DropTargetResetTime[count] - CurrentTime) / 32;
          if (timeUntilNext < 50) timeUntilNext = 50;
          DropTargetHurryTime[count] = CurrentTime + timeUntilNext;
          DropTargetHurryLamp[count] ^= 1;
//          if (DropTargetHurryLamp[count]) PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HURRY);
        }
      }
    }
  }
}


void UpdateTimedKicks() {
  byte numTimedKicks = 0;
  boolean ballKicked = false;

  for (byte count = 0; count < 3; count++) {
    if (LockKickTime[count]) {
      if (CurrentTime > LockKickTime[count]) {
        LockKickTime[count] = 0;
        RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);

        // Now we have to reset the upper lock switch debounce buffer
        // so that it registers if a new ball falls into it.
        byte machineLockBitToTurnOff = LOCK_3_ENGAGED;
        for (int i = 2; i >= 0; i--) {
          if (UpperLockSwitchState[i]) {
            UpperLockSwitchDownTime[i] = 0;
            UpperLockSwitchState[i] = false;
            MachineLocks &= ~machineLockBitToTurnOff;
            break;
          }
          machineLockBitToTurnOff /= 2;
        }

        ballKicked = true;
        NumberOfBallsInPlay += 1;
        NumberOfBallsLocked = CountBits(MachineLocks & LOCKS_ENGAGED_MASK);
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Num BIP +1 to %d b/c UpdateTimedKicks leaving Upper Switches = %d, %d, %d\n", NumberOfBallsInPlay, UpperLockSwitchState[0], UpperLockSwitchState[1], UpperLockSwitchState[2]);
          Serial.write(buf);
        }
        //        if (NumberOfBallsLocked) NumberOfBallsLocked -= 1;
      }
      numTimedKicks += 1;
    }
  }

  if (numTimedKicks == 1 && ballKicked) {
    // We have kicked the last held ball, so the jackpot opportunity is gone
  }

  if (SaucerKickTime && CurrentTime > SaucerKickTime) {
    SaucerKickTime = 0;
    RPU_PushToSolenoidStack(SOL_SAUCER, 12, true);
  }
}


byte ResetAllDropTargets(boolean onlyResetClearedBanks = false) {

  byte numBanksCleared = 0;

  if (DropTargetsUL.CheckIfBankCleared()) {
    if (onlyResetClearedBanks) DropTargetsUL.ResetDropTargets(CurrentTime + 200, true);
    numBanksCleared += 1;
  }
  if (DropTargetsUR.CheckIfBankCleared()) {
    if (onlyResetClearedBanks) DropTargetsUR.ResetDropTargets(CurrentTime + 400, true);
    numBanksCleared += 1;
  }
  if (DropTargetsLL.CheckIfBankCleared()) {
    if (onlyResetClearedBanks) DropTargetsLL.ResetDropTargets(CurrentTime + 600, true);
    numBanksCleared += 1;
  }
  if (DropTargetsLR.CheckIfBankCleared()) {
    if (onlyResetClearedBanks) DropTargetsLR.ResetDropTargets(CurrentTime + 800, true);
    numBanksCleared += 1;
  }


  if (!onlyResetClearedBanks) {
    if (DropTargetsUL.GetStatus(false)) DropTargetsUL.ResetDropTargets(CurrentTime + 200, true);
    if (DropTargetsUR.GetStatus(false)) DropTargetsUR.ResetDropTargets(CurrentTime + 400, true);
    if (DropTargetsLL.GetStatus(false)) DropTargetsLL.ResetDropTargets(CurrentTime + 600, true);
    if (DropTargetsLR.GetStatus(false)) DropTargetsLR.ResetDropTargets(CurrentTime + 800, true);
  }

  for (byte count = 0; count < 4; count++) {
    DropTargetResetTime[count] = 0;
  }

  return numBanksCleared;
}


void SetMagnetState(byte magnetNum, boolean magnetOn = true) {
  RPU_SetContinuousSolenoid(magnetOn, magnetNum ? SOL_RIGHT_MAGNA_SAVE : SOL_LEFT_MAGNA_SAVE);
}


byte GameModeStage;
boolean DisplaysNeedRefreshing = false;
boolean SawMagnetButonUp;
unsigned long LastTimePromptPlayed = 0;
unsigned short CurrentBattleLetterPosition = 0xFF;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
unsigned long LastLeftSpeakerMagnetAnimationTime = 0;
unsigned long LastRightSpeakerMagnetAnimationTime = 0;
#endif


#define NUM_GI_FLASH_SEQUENCE_ENTRIES 10
byte GIFlashIndex = 0;
unsigned long GIFlashTimer = 0;
unsigned int GIFlashChangeState[NUM_GI_FLASH_SEQUENCE_ENTRIES] = {1000, 1250, 2000, 2250, 3000, 3300, 3500, 3900, 5000, 6500};

// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  boolean specialAnimationRunning = false;

  UpdateDropTargets();
  UpdateComboStatus();
  UpdateTimedKicks();
  UpdateGI();
  
  if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
    if (LastSaucerHitTime==0) {
      LastSaucerHitTime = CurrentTime;
    } else {
      if (LastSaucerHitTime>1 && CurrentTime>(LastSaucerHitTime+250)) {
        HandleSaucer();
        LastSaucerHitTime = 1;
      }      
    }
  } else {
    LastSaucerHitTime = 0;
  }

//  if ((CurrentTime - LastSwitchHitTime) > 3000) TimersPaused = true;
//  else TimersPaused = false;
  TimersPaused = false;

  if (PlayLockKickWarningTime && CurrentTime>PlayLockKickWarningTime) {
    PlayLockKickWarningTime = 0;
    PlaySoundEffect(SOUND_EFFECT_EJECT_WARNING);
  }

  if (PlaySaucerKickWarningTime && CurrentTime>PlaySaucerKickWarningTime) {
    PlaySaucerKickWarningTime = 0;
    PlaySoundEffect(SOUND_EFFECT_EJECT_WARNING);
  }

  if (CurrentTime > SpinnerLitUntil) {
    SpinnerLitUntil = 0;
    SpinnerStatus = 0;
  }

  if (LastPopBumperHit && CurrentTime > (LastPopBumperHit + 1000)) {
    LastPopBumperHit = 0;
  }

  if (BonusXCollectReminder && CurrentTime > BonusXCollectReminder) {
    RemindBonusXCollect();
  }

  if (ExtraBallAwardStartTime && CurrentTime > (ExtraBallAwardStartTime + 5000)) {
    ExtraBallAwardStartTime = 0;
  }

  if (LevitateMagnetOnTime && (CurrentTime > LevitateMagnetOnTime)) {
    LevitateMagnetOnTime = 0;
    LevitateMagnetOffTime = CurrentTime + 1250;
    SetMagnetState(0, true);
    PlaySoundEffect(SOUND_EFFECT_LEVITATE);
  }

  if (LevitateMagnetOffTime && (CurrentTime > LevitateMagnetOffTime)) {
    if (KingsChallengeRunning & KINGS_CHALLENGE_LEVITATE) {
      LevitateMagnetOnTime = CurrentTime + 1000;
    }
    LevitateMagnetOffTime = 0;
    SetMagnetState(0, false);
    Audio.StopSound(SOUND_EFFECT_LEVITATE);
  }

  if (LastTimeLeftMagnetOn && CurrentTime>(LastTimeLeftMagnetOn+2000)) {
    LastTimeLeftMagnetOn = 0;
  }

  if (LastTimeRightMagnetOn && CurrentTime>(LastTimeRightMagnetOn+2000)) {
    LastTimeRightMagnetOn = 0;
  }

  boolean waitingForKick = false;
  if (LockKickTime[0] || LockKickTime[1] || LockKickTime[2]) waitingForKick = true;


  if (MagnaSaveAvailable) {
    boolean neitherMagnetOn = true;

    // manage magnets on/off
    unsigned long timeOnDelta = CurrentTime - LastTimeThroughLoop;
    if (RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON)) {
      if (MagnaStatusLeft[CurrentPlayer]) {
        if (timeOnDelta > MagnaStatusLeft[CurrentPlayer]) {
          MagnaStatusLeft[CurrentPlayer] = 0;
        } else {
          MagnaStatusLeft[CurrentPlayer] -= timeOnDelta;
        }
        LastTimeLeftMagnetOn = CurrentTime;
        SetMagnetState(0, true);
        neitherMagnetOn = false;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        if (CurrentTime>(LastLeftSpeakerMagnetAnimationTime+1000)) {
          LastLeftSpeakerMagnetAnimationTime = CurrentTime;
          TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 120, 0, 150, 1000);
          TopperALB.PlayAnimation(GI_2_FLICKER_END_PIXEL, 120, 0, 150, 1000);
        }
#endif
        if (!MagnaSoundOn) {
          PlaySoundEffect(SOUND_EFFECT_MAGNET);
          MagnaSoundOn = true;
        }
      } else {
        SetMagnetState(0, false);
      }
    } else {
      if (LevitateMagnetOffTime==0) SetMagnetState(0, false);
    }

    if (RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON)) {
      if (MagnaStatusRight[CurrentPlayer]) {
        if (timeOnDelta > MagnaStatusRight[CurrentPlayer]) {
          MagnaStatusRight[CurrentPlayer] = 0;
        } else {
          MagnaStatusRight[CurrentPlayer] -= timeOnDelta;
        }
        LastTimeRightMagnetOn = CurrentTime;
        SetMagnetState(1, true);
        neitherMagnetOn = false;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        if (CurrentTime>(LastRightSpeakerMagnetAnimationTime+1000)) {
          LastRightSpeakerMagnetAnimationTime = CurrentTime;
          TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 120, 0, 150, 1000);
          TopperALB.PlayAnimation(GI_2_FLICKER_START_PIXEL, 120, 0, 150, 1000);
        }
#endif
        if (!MagnaSoundOn) {
          PlaySoundEffect(SOUND_EFFECT_MAGNET);
          MagnaSoundOn = true;
        }
      } else {
        SetMagnetState(1, false);
      }
    } else {
      SetMagnetState(1, false);
    }

    if (neitherMagnetOn && MagnaSoundOn) {
      MagnaSoundOn = false;
      Audio.StopSound(SOUND_EFFECT_MAGNET);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.StopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0);
      TopperALB.StopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0);
      TopperALB.StopAnimation(GI_2_FLICKER_END_PIXEL);
      TopperALB.StopAnimation(GI_2_FLICKER_START_PIXEL);
#endif      
    }
  } else {
    SetMagnetState(0, false);
    SetMagnetState(1, false);
    if (MagnaSoundOn) {
      MagnaSoundOn = false;
      Audio.StopSound(SOUND_EFFECT_MAGNET);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.StopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0);
      TopperALB.StopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0);
      TopperALB.StopAnimation(GI_2_FLICKER_END_PIXEL);
      TopperALB.StopAnimation(GI_2_FLICKER_START_PIXEL);
#endif      
    }
  }


  switch ( GameMode ) {
    case GAME_MODE_WAIT_FOR_BALL_TO_RETURN:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        RPU_SetDisableFlippers();
        SetGeneralIlluminationOn(false);
        MagnaSaveAvailable = false;

        // Tell the machine that we haven't started the ball yet
        BallFirstSwitchHitTime = 0;
      }
      if (CountBallsInTrough()) {
        PutBallInPlay();
        SetGameMode(GAME_MODE_BALL_IN_SHOOTER_LANE);
      }
      break;
    case GAME_MODE_BALL_IN_SHOOTER_LANE:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        SetGeneralIlluminationOn(true);
        RPU_SetDisableFlippers(false);
        MagnaSaveAvailable = false;
      }

      if (RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
        SetGameMode(WaitingForBallGameMode);
      }
      break;
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
#endif
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastTimePromptPlayed = 0;
        GameModeStage = 0;
        SetGeneralIlluminationOn(true);
        MagnaSaveAvailable = false;
      }

      if (GameModeStage == 0 && RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
        GameModeStage = 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 0, 50);
#endif    
      } else if (GameModeStage == 1 && !RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
        //PlaySoundEffect(SOUND_EFFECT_LEFT_SHOOTER_LANE);
        Audio.PlayBackgroundSong(SOUND_EFFECT_RALLY_PLUNGE);
        GameModeStage = 2;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
//        TopperALB.LoopAnimation(TOPPER_LEFT_TO_RIGHT_COLOR_0, 70, 70, 250);
#endif    
      } else if (GameModeStage == 2 && RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
        GameModeStage = 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 0, 50);
        //TopperALB.PlayAnimation(RIGHT_TO_LEFT_SPEAKER_COLOR_0, 220, 0, 50);
#endif    
        //PlaySoundEffect(SOUND_EFFECT_RETURN_TO_SHOOTER_LANE);
        byte rallyNum = CurrentBallInPlay - 1;
        if (BallsPerGame > 3) rallyNum = (CurrentBallInPlay > 1) ? ((CurrentBallInPlay == 5) ? 2 : 1) : 0;
        PlayBackgroundSong(SOUND_EFFECT_RALLY_SONG_1 + rallyNum);
      }

      // If we've seen a tilt before plunge, then
      // we can show a countdown timer here
      if (LastTiltWarningTime) {
        if ( CurrentTime > (LastTiltWarningTime + 30000) ) {
          LastTiltWarningTime = 0;
        } else {
          byte secondsSinceWarning = (CurrentTime - LastTiltWarningTime) / 1000;
          for (byte count = 0; count < RPU_NUMBER_OF_PLAYERS_ALLOWED; count++) {
            if (count == CurrentPlayer) Display_OverrideScoreDisplay(count%RPU_NUMBER_OF_PLAYER_DISPLAYS, 30 - secondsSinceWarning, DISPLAY_OVERRIDE_ANIMATION_CENTER);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        Display_ClearOverride(0xFF);
      }
      
      if (LastTimePromptPlayed==0 && (RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) || RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON))) {
        LastTimePromptPlayed = CurrentTime;
        QueueNotification(SOUND_EFFECT_VP_CURRENT_JACKPOT, 8);        
      } else {
        if (CurrentTime>(LastTimePromptPlayed+3000)) {
          LastTimePromptPlayed = 0;
          DisplaysNeedRefreshing = true;
        } else {
          Display_OverrideScoreDisplay(CurrentPlayer, CombatJackpot[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
        }
      }

      if (BallFirstSwitchHitTime != 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimationByArea(MESSAGE_AREA_TOPPER);
        TopperALB.StopAnimationByArea(MESSAGE_AREA_SPEAKER);
        TopperALB.StopAnimationByArea(MESSAGE_AREA_LEFT_SPEAKER);
        TopperALB.StopAnimationByArea(MESSAGE_AREA_RIGHT_SPEAKER);
        TopperALB.PlayAnimation(TOPPER_FIRE);
#endif
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        PlaySoundEffect(SOUND_EFFECT_HORSE_NEIGHING);
        byte songToPlay;
        if (BallsPerGame > 3) {
          songToPlay = (((CurrentBallInPlay - 1) * 2) + CurrentTime % 2) % NUM_BACKGROUND_SONGS;
        } else {
          songToPlay = (((CurrentBallInPlay - 1) * 3) + CurrentTime % 5) % NUM_BACKGROUND_SONGS;
        }
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_1 + songToPlay);
      }
      break;

    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimationByArea(MESSAGE_AREA_TOPPER);
        TopperALB.StopAnimationByArea(MESSAGE_AREA_LEFT_SPEAKER);
        TopperALB.StopAnimationByArea(MESSAGE_AREA_RIGHT_SPEAKER);
        if (KingsChallengeEndTime) {
          TopperALB.LoopAnimation(TOPPER_LOOP_COLOR_0, 80, 15, 0);
        } else if (KingsChallengeStatus[CurrentPlayer] & KINGS_CHALLENGE_AVAILABLE) {
          TopperALB.LoopAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 80, 15, 0);
          TopperALB.LoopAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 80, 15, 0);          
        }
#endif
        TripleCombatJackpotsAvailable = 0;
        GameModeStartTime = CurrentTime;
        DisplaysNeedRefreshing = true;
        TicksCountedTowardsStatus = 0;
        IdleMode = IDLE_MODE_NONE;
        if (DEBUG_MESSAGES) {
          Serial.write("Entering unstructured play\n");
        }
        SetGeneralIlluminationOn(true);
        MagnaSaveAvailable = true;
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Unstructured: Player lock = 0x%02X\n", PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
        GameModeStage = 0;
      }

//      if (TimersPaused && IdleModeEnabled) {
//        TicksCountedTowardsStatus += (CurrentTime - LastTimeThroughLoop);
//        AnnounceStatus();
//      } else {
//        TicksCountedTowardsStatus = 0;
//        IdleMode = IDLE_MODE_NONE;
//      }
      
      if (PlayfieldMultiplierExpiration) {
        // Playfield X value is only reset during unstructured play
        if (CurrentTime > PlayfieldMultiplierExpiration) {
          PlayfieldMultiplierExpiration = 0;
          if (PlayfieldMultiplier > 1) QueueNotification(SOUND_EFFECT_VP_RETURN_TO_1X, 1);
          PlayfieldMultiplier = 1;
        } else {
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, PlayfieldMultiplier, DISPLAY_OVERRIDE_SYMMETRIC_BOUNCE);
          }
          DisplaysNeedRefreshing = true;
        }
      } else if (DisplaysNeedRefreshing) {
        DisplaysNeedRefreshing = false;
        Display_ClearOverride(0xFF);
      }

      if (KingsChallengeEndTime) {
        // King's Challenge only expires during unstructured play
        if (CurrentTime>KingsChallengeEndTime) {
          // Payoff Bonus
          Display_StartScoreAnimation(KingsChallengeBonus * PlayfieldMultiplier, true);

          KingsChallengeStatus[CurrentPlayer] |= (KingsChallengeRunning * 0x10);
          KingsChallengeEndTime = 0;
          KingsChallengeRunning = 0;
          Display_ClearOverride(0xFF);
          GameModeStage = 0;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.StopAnimationByArea(MESSAGE_AREA_TOPPER);
          TopperALB.StopAnimationByArea(MESSAGE_AREA_SPEAKER);
          TopperALB.StopAnimationByArea(MESSAGE_AREA_LEFT_SPEAKER);
          TopperALB.StopAnimationByArea(MESSAGE_AREA_RIGHT_SPEAKER);
#endif
        } else {
          if ( (KingsChallengeEndTime - CurrentTime) < 10000 ) {
            if (GameModeStage==0) {
              GameModeStage = 10;
              PlaySoundEffect(SOUND_EFFECT_10_SECONDS_LEFT);
            }
            byte timeLeft = (KingsChallengeEndTime - CurrentTime) / 1000;            
            Display_OverrideScoreDisplay(CurrentPlayer, timeLeft, DISPLAY_OVERRIDE_ANIMATION_CENTER);
            if ( timeLeft<(GameModeStage-1) ) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
              TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 80, 15, 0);
              TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 80, 15, 0);
#endif
              if (GameModeStage%2) PlaySoundEffect(SOUND_EFFECT_TOM_HIT_LEFT);
              else PlaySoundEffect(SOUND_EFFECT_TOM_HIT_RIGHT);
              GameModeStage = timeLeft+1;
            }
          }

          if (KingsChallengeBonusChangedTime) {
            if (CurrentTime>(KingsChallengeBonusChangedTime+3000)) {
              KingsChallengeBonusChangedTime = 0;
            } else {
              for (byte count = 0; count < 4; count++) {
                if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, KingsChallengeBonus, DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
              }
              DisplaysNeedRefreshing = true;
            }
          }
        }
      }

      break;
    case GAME_MODE_OFFER_SINGLE_COMBAT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 8000;
        Audio.StopAllNotifications();
        if (SingleCombatLevelCompleted[CurrentPlayer] == 0) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_SINGLE, 10);
        } else if (SingleCombatLevelCompleted[CurrentPlayer] == 1) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_SINGLE_PART_2, 10);
        } else if (SingleCombatLevelCompleted[CurrentPlayer] == 2) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_SINGLE_PART_3, 10);
        } else if (SingleCombatLevelCompleted[CurrentPlayer] == 3) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_SINGLE, 10);
        }
        MagnaSaveAvailable = false;
//        if (DEBUG_MESSAGES) Serial.write("Offer single combat\n");
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimationByArea(MESSAGE_AREA_TOPPER);
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 0, 220);
        TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
#endif        
      }

      for (byte count = 0; count < 4; count++) {
        byte playerScorePhase = ((CurrentTime-GameModeStartTime)/1000)%2;
        if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, DISPLAY_OVERRIDE_ANIMATION_CENTER);
        else if (playerScorePhase && CombatJackpot[CurrentPlayer]) Display_OverrideScoreDisplay(count, CombatJackpot[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
        else Display_ClearOverride(count);
      }

      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        QueueNotification(SOUND_EFFECT_VP_BALL_LOCKED, 8);
        LockBall();
        LockManagementInProgress = false;
        if (PutBallInPlay()) {
          WaitingForBallGameMode = GAME_MODE_UNSTRUCTURED_PLAY;
          SetGameMode(GAME_MODE_BALL_IN_SHOOTER_LANE);
        } else {
          NumberOfBallsInPlay = 1;
          SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        }
      } else if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) && RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        Audio.StopAllNotifications();
        Display_ClearOverride(0xFF);
        SetGameMode(GAME_MODE_SINGLE_COMBAT_START);
        //        RemoveTopQualifiedFlag();
        PlayerLockStatus[CurrentPlayer] = (PlayerLockStatus[CurrentPlayer] & LOCKS_AVAILABLE_MASK) / 2;
      }
      TimersPaused = true;
      break;

    case GAME_MODE_SINGLE_COMBAT_START:
      if (GameModeStartTime == 0) {
        Audio.StopAllMusic();
        MagnaSoundOn = false;
        GameModeStartTime = CurrentTime;
        SetGeneralIlluminationOn(true);
        RPU_SetDisableFlippers(false);
        GameModeStage = 0;
        CombatJackpotReady = false;
        CombatSuperJackpotReady = false;

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 0, 220);
        TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
        TopperALB.PlayAnimation(GI_0_PULSE_COLOR, 220, 0, 220, 4000);
        TopperALB.PlayAnimation(GI_1_PULSE_COLOR, 220, 0, 220, 4000);
        TopperALB.PlayAnimation(GI_2_PULSE_COLOR, 220, 0, 220, 4000);
#endif

        GameModeEndTime = CurrentTime + 6000;
        ResetAllDropTargets();
        MagnaSaveAvailable = false;
        SawMagnetButonUp = false;
        LastTimePromptPlayed = CurrentTime;
        Audio.SetSoundFXDuckingGain(5);
        PlaySoundEffect(SOUND_EFFECT_RISING_WARNING);
      }

      if (LastTimePromptPlayed && CurrentTime>(LastTimePromptPlayed+3000)) {
        LastTimePromptPlayed = 0;
        if (SingleCombatLevelCompleted[CurrentPlayer] == 0) {
          QueueNotification(SOUND_EFFECT_VP_SINGLE_HINT_0, 10);
        } else if (SingleCombatLevelCompleted[CurrentPlayer] == 1) {
          QueueNotification(SOUND_EFFECT_VP_SINGLE_HINT_PART_1, 10);
        } else {
          QueueNotification(SOUND_EFFECT_VP_SINGLE_HINT_PART_2, 10);
        }
      }
//      if (CurrentTime>(LastTimePromptPlayed+250)) {
//        LastTimePromptPlayed = CurrentTime;
//        Audio.PlaySoundCardWhenPossible(3, CurrentTime, 0, 1000, 10);
//      }

      specialAnimationRunning = true;
      if (GameModeStage==0) {
        GameModeStage = 1;
        RPU_TurnOffAllLamps();
        SetGeneralIlluminationOn(false);
      } else {
        byte lampPhase = (CurrentTime-GameModeStartTime) / 200;
        FlashAnimationSteps(2, lampPhase, 50);
      }

      if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) && RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        if (SawMagnetButonUp) {
          Audio.StopAllNotifications();
          GameModeEndTime = CurrentTime - 1;
        }
      } else {
        SawMagnetButonUp = true;
      }

      if (CurrentTime > GameModeEndTime) {
        QueueNotification(SOUND_EFFECT_VP_SINGLE_COMBAT, 8);
        switch (SingleCombatNumSeconds) {
          case 30: QueueNotification(SOUND_EFFECT_VP_30_SECONDS, 8); break;
          case 45: QueueNotification(SOUND_EFFECT_VP_45_SECONDS, 8); break;
          case 60: QueueNotification(SOUND_EFFECT_VP_60_SECONDS, 8); break;
          case 75: QueueNotification(SOUND_EFFECT_VP_75_SECONDS, 8); break;
          case 90: QueueNotification(SOUND_EFFECT_VP_90_SECONDS, 8); break;
          case 120: QueueNotification(SOUND_EFFECT_VP_120_SECONDS, 8); break;
        }
        SetGameMode(GAME_MODE_SINGLE_COMBAT);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_SINGLE_COMBAT:
      if (GameModeStartTime == 0) {
        Audio.SetSoundFXDuckingGain(15);
        ReturnToFightAlreadyPlayed = false;
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1);
        RPU_TurnOffAllLamps();
        SetGeneralIlluminationOn(true);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_LOOP_COLOR_0, 220, 0, 220);
#endif        
        GameModeStartTime = CurrentTime;
        if (DoubleTimers) GameModeEndTime = CurrentTime + ((unsigned long)SingleCombatNumSeconds) * 2000;
        else GameModeEndTime = CurrentTime + ((unsigned long)SingleCombatNumSeconds) * 1000;
        MagnaSaveAvailable = true;
        LockKickTime[0] = CurrentTime + 100;
        OverrideGeneralIllumination(CurrentTime, 50);
        OverrideGeneralIllumination(CurrentTime+100, 50);
        OverrideGeneralIllumination(CurrentTime+200, 50);
        LockManagementInProgress = false;
        NumberOfBallsInPlay = 0;
        CombatBankFlags = 0;
        CombatJackpotReady = false;
        CombatSuperJackpotReady = false;
        JackpotIncreasedTime = 0;
        if (DEBUG_MESSAGES) {
          char buf[128];
          sprintf(buf, "Single: Player lock = 0x%02X\n", PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
        GameModeStage = 0;
        if (BallSaveOnCombatModes) {
          BallSaveEndTime = CurrentTime + ((unsigned long)BallSaveOnCombatModes * 1000);
        }
      }

      if (JackpotIncreasedTime) {
        if (CurrentTime > (JackpotIncreasedTime + 2000)) {
          JackpotIncreasedTime = 0;
        } else {
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, CombatJackpot[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
          }
        }
      } else {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, DISPLAY_OVERRIDE_ANIMATION_CENTER);
        }
      }

      if ( (GameModeEndTime-CurrentTime)<10000 ) {
        if (GameModeStage==0) {
          GameModeStage = 10;
          if (!CombatJackpotReady) {
            PlaySoundEffect(SOUND_EFFECT_10_SECONDS_LEFT);
          } else {
            QueueNotification(SOUND_EFFECT_VP_SINGLE_TEN_SECONDS_TO_HIT_SAUCER, 8);
          }
        } else {
          byte timeLeft = (GameModeEndTime-CurrentTime)/1000;
          if ( timeLeft<(GameModeStage-1) ) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            if (CombatJackpotReady) {
              TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 0);
              TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 0);
            } else {
              TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 220);
              TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
            }
#endif
            if (GameModeStage%2) PlaySoundEffect(SOUND_EFFECT_TOM_HIT_LEFT);
            else PlaySoundEffect(SOUND_EFFECT_TOM_HIT_RIGHT);
            GameModeStage = timeLeft+1;
          }
        }
      }

      if (CurrentTime > GameModeEndTime) {
        SetGameMode(GAME_MODE_SINGLE_COMBAT_LOST);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_SINGLE_COMBAT_WON:
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_FLASH_COLOR_0, 220, 0, 220);
#endif        
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_FANFARE_1);
        PlaySoundEffect(SOUND_EFFECT_CROWD_CHEERING);
        if (!SaucerLitFromCombo) {
          QueueNotification(SOUND_EFFECT_VP_JACKPOT, 8);
          CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_1;
          if (CombatBankFlags==0x0F) Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer] * 2, true);
          else Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer], true);
        } else if (LeftInlaneLitFromCombo) {
          QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 8);
          CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_1*10;
          if (CombatBankFlags==0x0F) Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer] * 2, true);
          else Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer], true);
        } else {
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 8);
          CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_1*2;
          if (CombatBankFlags==0x0F) Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer] * 2, true);
          else Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer], true);
        } 

        Display_ClearOverride(0xFF);
        GameModeStage = SingleCombatLevelCompleted[CurrentPlayer];
        if (SingleCombatLevelCompleted[CurrentPlayer] < 3) {
          SingleCombatLevelCompleted[CurrentPlayer] += 1;
        }

      }

      if (CurrentTime > GameModeEndTime) {
        RPU_PushToSolenoidStack(SOL_SAUCER, 16, true);
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2 + SingleCombatLevelCompleted[CurrentPlayer]);
        if (SingleCombatLevelCompleted[CurrentPlayer] > GameModeStage) {
          if (SingleCombatLevelCompleted[CurrentPlayer] == 1) QueueNotification(SOUND_EFFECT_VP_SINGLE_COMBAT_PART_1_COMPLETE, 8);
          if (SingleCombatLevelCompleted[CurrentPlayer] == 2) QueueNotification(SOUND_EFFECT_VP_SINGLE_COMBAT_PART_2_COMPLETE, 8);
          if (SingleCombatLevelCompleted[CurrentPlayer] == 3) QueueNotification(SOUND_EFFECT_VP_SINGLE_COMBAT_PART_3_COMPLETE, 8);
        }
        ResetAllDropTargets();
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
#endif        
      }
      TimersPaused = true;
      break;

    case GAME_MODE_SINGLE_COMBAT_LOST:
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
#endif        
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_BOOING_3);
        Display_ClearOverride(0xFF);
      }

      if (CurrentTime > GameModeEndTime) {
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        QueueNotification(SOUND_EFFECT_VP_SINGLE_LOST, 8);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_OFFER_DOUBLE_COMBAT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 8000;
        Audio.StopAllNotifications();
        if (DoubleCombatLevelCompleted[CurrentPlayer] == 0) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_DOUBLE, 10);
        } else if (DoubleCombatLevelCompleted[CurrentPlayer] == 1) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_DOUBLE_PART_2, 10);
        } else if (DoubleCombatLevelCompleted[CurrentPlayer] == 2) {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_DOUBLE_PART_3, 10);
        } else {
          QueueNotification(SOUND_EFFECT_VP_PRESS_FOR_DOUBLE, 10);
        }
        MagnaSaveAvailable = false;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimationByArea(MESSAGE_AREA_TOPPER);
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 120, 0);
        TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 120, 0);
#endif
      }

      for (byte count = 0; count < 4; count++) {
        byte playerScorePhase = ((CurrentTime-GameModeStartTime)/1000)%2;
        if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, DISPLAY_OVERRIDE_ANIMATION_CENTER);
        else if (playerScorePhase && CombatJackpot[CurrentPlayer]) Display_OverrideScoreDisplay(count, CombatJackpot[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
        else Display_ClearOverride(count);
      }

      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        QueueNotification(SOUND_EFFECT_VP_BALL_LOCKED, 8);
        LockBall();
        LockManagementInProgress = false;
        if (PutBallInPlay()) {
          WaitingForBallGameMode = GAME_MODE_UNSTRUCTURED_PLAY;
          SetGameMode(GAME_MODE_BALL_IN_SHOOTER_LANE);
        } else {
          NumberOfBallsInPlay = 1;
          SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        }
      } else if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) && RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        Audio.StopAllNotifications();
        PlayerLockStatus[CurrentPlayer] = (PlayerLockStatus[CurrentPlayer] & LOCKS_AVAILABLE_MASK) / 4;
        SetGameMode(GAME_MODE_DOUBLE_COMBAT_START);
        Display_ClearOverride(0xFF);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT_START:
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 220, 120, 0);
        TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 220, 120, 0);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 120, 0);
        TopperALB.PlayAnimation(GI_0_PULSE_COLOR, 220, 120, 0, 4000);
        TopperALB.PlayAnimation(GI_1_PULSE_COLOR, 220, 120, 0, 4000);
        TopperALB.PlayAnimation(GI_2_PULSE_COLOR, 220, 120, 0, 4000);
#endif
        Audio.SetSoundFXDuckingGain(5);
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_RISING_WARNING);
        GameModeStartTime = CurrentTime;
        SetGeneralIlluminationOn(true);
        RPU_SetDisableFlippers(false);
        GameModeStage = 0;
        CombatBankFlags = 0;
        CombatJackpotReady = false;
        CombatSuperJackpotReady = false;
        CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_2;
        JackpotBeforeCombat = CombatJackpot[CurrentPlayer];
        GameModeEndTime = CurrentTime + 6000;
        ResetAllDropTargets();
        MagnaSaveAvailable = false;
        SawMagnetButonUp = false;
        LastTimePromptPlayed = CurrentTime;
      }

      if (LastTimePromptPlayed && CurrentTime>(LastTimePromptPlayed+3000)) {
        LastTimePromptPlayed = 0;
        if (DoubleCombatLevelCompleted[CurrentPlayer] == 0) {
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_HINT_PART_1, 10);
        } else if (DoubleCombatLevelCompleted[CurrentPlayer] == 1) {
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_HINT_2_OR_3, 10);
        } else if (DoubleCombatLevelCompleted[CurrentPlayer] == 2) {
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_HINT_2_OR_3, 10);
        } else {
          QueueNotification(SOUND_EFFECT_VP_DOUBLE_HINT_PART_1, 10);
        }
      }

      specialAnimationRunning = true;
      if (GameModeStage==0) {
        GameModeStage = 1;
        RPU_TurnOffAllLamps();
        SetGeneralIlluminationOn(false);
      } else {
        byte lampPhase = (CurrentTime-GameModeStartTime) / 200;
        FlashAnimationSteps(1, lampPhase, 50);
      }

      if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) && RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        if (SawMagnetButonUp) {
          Audio.StopAllNotifications();
          GameModeEndTime = CurrentTime;
        }
      } else {
        SawMagnetButonUp = true;
      }

      if (CurrentTime >= GameModeEndTime) {
        SetGameMode(GAME_MODE_DOUBLE_COMBAT);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT:
      if (GameModeStartTime == 0) {
        Audio.SetSoundFXDuckingGain(15);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_LOOP_COLOR_0, 220, 120, 0);
#endif
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1 + 5);
        RPU_TurnOffAllLamps();
        SetGeneralIlluminationOn(true);
        GameModeStartTime = CurrentTime;
        ReturnToFightAlreadyPlayed = false;
        MagnaSaveAvailable = true;
        LockKickTime[0] = CurrentTime + 100;
        LockKickTime[1] = CurrentTime + 1100;
        OverrideGeneralIllumination(CurrentTime, 50);
        OverrideGeneralIllumination(CurrentTime+100, 50);
        OverrideGeneralIllumination(CurrentTime+200, 50);
        OverrideGeneralIllumination(CurrentTime+1000, 50);
        OverrideGeneralIllumination(CurrentTime+1100, 50);
        OverrideGeneralIllumination(CurrentTime+1200, 50);
        if (NumberOfBallsLocked) NumberOfBallsLocked -= 1;
        LockManagementInProgress = false;
        waitingForKick = true;
        NumberOfBallsInPlay = 0;
        GameModeStage = 0;
      }

      if (!waitingForKick && NumberOfBallsInPlay <= 1) {
        // If the player drops down to 1 ball,
        // put the mode on a timer
        if (GameModeEndTime==0) {
          if (DoubleTimers) GameModeEndTime = CurrentTime + ((unsigned long)DoubleCombatNumSeconds) * 2000;
          else GameModeEndTime = CurrentTime + ((unsigned long)DoubleCombatNumSeconds) * 1000;
        }
      }

      if (CurrentTime<GameModeEndTime && GameModeEndTime) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, DISPLAY_OVERRIDE_ANIMATION_CENTER);
        }        
      }

      if ( GameModeEndTime && CurrentTime<GameModeEndTime && (GameModeEndTime-CurrentTime)<10000 ) {
        if (GameModeStage==0) {
          GameModeStage = 10;
          PlaySoundEffect(SOUND_EFFECT_10_SECONDS_LEFT);
        } else {
          byte timeLeft = (GameModeEndTime-CurrentTime)/1000;
          if ( timeLeft<(GameModeStage-1) ) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 0);
            TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 0);
#endif
            if (GameModeStage%2) PlaySoundEffect(SOUND_EFFECT_TOM_HIT_LEFT);
            else PlaySoundEffect(SOUND_EFFECT_TOM_HIT_RIGHT);
            GameModeStage = timeLeft+1;
          }
        }
      }

      if (GameModeEndTime && (CurrentTime > GameModeEndTime)) {
        Display_ClearOverride(0xFF);
        if (JackpotBeforeCombat != CombatJackpot[CurrentPlayer]) {
          if (DoubleCombatLevelCompleted[CurrentPlayer]==0) SetGameMode(GAME_MODE_DOUBLE_COMBAT_FIRST_WIN);
          else SetGameMode(GAME_MODE_DOUBLE_COMBAT_LEVEL_INCREASED);
        } else {
          // Double combat over but they have a jackpot, they either tied or increased
          if (DoubleCombatLevelCompleted[CurrentPlayer]==0) SetGameMode(GAME_MODE_DOUBLE_COMBAT_LOST);
          else SetGameMode(GAME_MODE_DOUBLE_COMBAT_LEVEL_SAME);
        }
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT_LOST:
      if (GameModeStartTime == 0) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 250, 0, 0);
#endif        
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_LOST, 7);
      }

      if (CurrentTime > GameModeEndTime) {
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT_FIRST_WIN:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_FANFARE_2);
        PlaySoundEffect(SOUND_EFFECT_CROWD_CHEERING);
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_COMBAT_FIRST_VICTORY, 7);
        DoubleCombatLevelCompleted[CurrentPlayer] += 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_BIG_LIGHTNING_0);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT_LEVEL_INCREASED:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        PlaySoundEffect(SOUND_EFFECT_CROWD_CHEERING);
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_COMBAT_LEVEL_INCREASED, 7);
        DoubleCombatLevelCompleted[CurrentPlayer] += 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_BIG_LIGHTNING_0);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_DOUBLE_COMBAT_LEVEL_SAME:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        QueueNotification(SOUND_EFFECT_VP_DOUBLE_COMBAT_LEVEL_SAME, 7);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 220, 120, 0);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_2);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT_START:
      if (GameModeStartTime == 0) {
        Audio.StopAllNotifications();
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_LIGHTNING_0);
        TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 0, 0, 240);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 0, 0, 240);
        TopperALB.PlayAnimation(GI_0_PULSE_COLOR, 0, 0, 240, 4000);
        TopperALB.PlayAnimation(GI_1_PULSE_COLOR, 0, 0, 240, 4000);
        TopperALB.PlayAnimation(GI_2_PULSE_COLOR, 0, 0, 240, 4000);
#endif        
        Audio.StopAllMusic();
        GameModeStartTime = CurrentTime;
        SetGeneralIlluminationOn(true);
        RPU_SetDisableFlippers(false);
        GameModeStage = 0;
        CombatBankFlags = 0;
        TripleCombatJackpotsAvailable = 0;
        CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_3;
        JackpotBeforeCombat = CombatJackpot[CurrentPlayer];

        GameModeEndTime = CurrentTime + 6000;
        ResetAllDropTargets();
        MagnaSaveAvailable = false;
        SawMagnetButonUp = false;
        Audio.SetSoundFXDuckingGain(5);
        PlaySoundEffect(SOUND_EFFECT_RISING_WARNING);
        LastTimePromptPlayed = CurrentTime;
      }

      if (LastTimePromptPlayed && CurrentTime>(LastTimePromptPlayed+3000)) {
        LastTimePromptPlayed = 0;
        if (TripleCombatLevelCompleted[CurrentPlayer] == 0) {
          QueueNotification(SOUND_EFFECT_VP_TRIPLE_HINT_0, 10);
        } else if (TripleCombatLevelCompleted[CurrentPlayer] == 1) {
          QueueNotification(SOUND_EFFECT_VP_TRIPLE_HINT_1, 10);
        } else if (TripleCombatLevelCompleted[CurrentPlayer] == 2) {
          QueueNotification(SOUND_EFFECT_VP_TRIPLE_HINT_2, 10);
        } else {
          QueueNotification(SOUND_EFFECT_VP_TRIPLE_HINT_3, 10);
        }
      }

      specialAnimationRunning = true;
      if (GameModeStage==0) {
        GameModeStage = 1;
        RPU_TurnOffAllLamps();
        SetGeneralIlluminationOn(false);
      } else {
        byte lampPhase = (CurrentTime-GameModeStartTime) / 200;
        FlashAnimationSteps(3, lampPhase, 50);
      }

      if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) && RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        if (SawMagnetButonUp) {
          Audio.StopAllNotifications();
          GameModeEndTime = CurrentTime;
        }
      } else {
        SawMagnetButonUp = true;
        byte playerScorePhase = ((CurrentTime-GameModeStartTime)/1000)%2;
        if (playerScorePhase && CombatJackpot[CurrentPlayer]) Display_OverrideScoreDisplay(CurrentPlayer, CombatJackpot[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_FLUTTER);
        else Display_ClearOverride(CurrentPlayer);
      }

      if (CurrentTime >= GameModeEndTime) {
        SetGameMode(GAME_MODE_TRIPLE_COMBAT);
        Display_ClearOverride(CurrentPlayer);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT:
      if (GameModeStartTime == 0) {
        Audio.SetSoundFXDuckingGain(15);
        GameModeStartTime = CurrentTime;
        PlayBackgroundSong(SOUND_EFFECT_BATTLE_SONG_1 + 2);
        MagnaSaveAvailable = true;
        LockKickTime[0] = CurrentTime + 100;
        LockKickTime[1] = CurrentTime + 1100;
        LockKickTime[2] = CurrentTime + 2100;
        OverrideGeneralIllumination(CurrentTime, 50);
        OverrideGeneralIllumination(CurrentTime+100, 50);
        OverrideGeneralIllumination(CurrentTime+200, 50);
        OverrideGeneralIllumination(CurrentTime+1000, 50);
        OverrideGeneralIllumination(CurrentTime+1100, 50);
        OverrideGeneralIllumination(CurrentTime+1200, 50);
        OverrideGeneralIllumination(CurrentTime+2000, 50);
        OverrideGeneralIllumination(CurrentTime+2100, 50);
        OverrideGeneralIllumination(CurrentTime+2200, 50);
        LockManagementInProgress = false;
        PlayerLockStatus[CurrentPlayer] = 0;
        NumberOfBallsLocked = 0;
        NumberOfBallsInPlay = 0;
        waitingForKick = true;
        SetGeneralIlluminationOn(true);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_LOOP_COLOR_0, 0, 0, 240);
#endif        
      }

      if (!waitingForKick && NumberOfBallsInPlay <= 1) {
        // If the player drops down to 1 ball,
        // put the mode on a timer
        if (GameModeEndTime==0) {
          if (DoubleTimers) GameModeEndTime = CurrentTime + ((unsigned long)TripleCombatNumSeconds) * 2000;
          else GameModeEndTime = CurrentTime + ((unsigned long)TripleCombatNumSeconds) * 1000;
        }
      }

      if (CurrentTime<GameModeEndTime && GameModeEndTime) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) Display_OverrideScoreDisplay(count, (GameModeEndTime - CurrentTime) / 1000, DISPLAY_OVERRIDE_ANIMATION_CENTER);
        }        
      }

      if ( GameModeEndTime && CurrentTime<GameModeEndTime && (GameModeEndTime-CurrentTime)<10000) {
        if (GameModeStage==0) {
          GameModeStage = 10;
          PlaySoundEffect(SOUND_EFFECT_10_SECONDS_LEFT);
        } else {
          byte timeLeft = (GameModeEndTime-CurrentTime)/1000;
          if ( timeLeft<(GameModeStage-1) ) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 0);
            TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 220, 0, 0);
#endif
            if (GameModeStage%2) PlaySoundEffect(SOUND_EFFECT_TOM_HIT_LEFT);
            else PlaySoundEffect(SOUND_EFFECT_TOM_HIT_RIGHT);
            GameModeStage = timeLeft+1;
          }
        }
      }

      if (GameModeEndTime && (CurrentTime >= GameModeEndTime)) {
        Display_ClearOverride(0xFF);
        if (JackpotBeforeCombat != CombatJackpot[CurrentPlayer]) {
          if (TripleCombatLevelCompleted[CurrentPlayer]==0) SetGameMode(GAME_MODE_TRIPLE_COMBAT_FIRST_WIN);
          else SetGameMode(GAME_MODE_TRIPLE_COMBAT_LEVEL_INCREASED);
        } else {
          // Double combat over but they have a jackpot, they either tied or increased
          if (TripleCombatLevelCompleted[CurrentPlayer]==0) SetGameMode(GAME_MODE_TRIPLE_COMBAT_LOST);
          else SetGameMode(GAME_MODE_TRIPLE_COMBAT_LEVEL_SAME);
        }
      }

      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT_LOST:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        QueueNotification(SOUND_EFFECT_VP_TRIPLE_LOST, 7);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 240, 0, 0);
#endif        
      }

      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        ResetAllDropTargets();
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT_FIRST_WIN:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_CROWD_CHEERING);
        PlaySoundEffect(SOUND_EFFECT_FANFARE_3);
        QueueNotification(SOUND_EFFECT_VP_TRIPLE_COMBAT_FIRST_VICTORY, 7);
        DoubleCombatLevelCompleted[CurrentPlayer] += 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_BIG_LIGHTNING_0);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_3);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT_LEVEL_INCREASED:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        Audio.StopAllMusic();
        PlaySoundEffect(SOUND_EFFECT_CROWD_CHEERING);
        QueueNotification(SOUND_EFFECT_VP_TRIPLE_COMBAT_LEVEL_INCREASED, 7);
        DoubleCombatLevelCompleted[CurrentPlayer] += 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_BIG_LIGHTNING_0);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_3);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;

    case GAME_MODE_TRIPLE_COMBAT_LEVEL_SAME:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        QueueNotification(SOUND_EFFECT_VP_TRIPLE_COMBAT_LEVEL_SAME, 7);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 0, 0, 240);
#endif        
      }
      if (CurrentTime > GameModeEndTime) {
        Display_ClearOverride(0xFF);
        ResetAllDropTargets();
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_SONG_3);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      TimersPaused = true;
      break;
      
    case GAME_MODE_KINGS_CHALLENGE_START:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 5000;
        byte numChallengesRunning = 0;
        byte challengeFlag = KINGS_CHALLENGE_1_QUALIFIED;
        for (byte count=0; count<4; count++) {
          if (KingsChallengeStatus[CurrentPlayer] & challengeFlag) {
            QueueNotification(SOUND_EFFECT_VP_KINGS_CHALLENGE_1 + numChallengesRunning, 10);
            numChallengesRunning += 1;
            QueueNotification(SOUND_EFFECT_VP_KINGS_CHALLENGE_JOUST + count, 10);

            // decrease drop target progress 
            NumDropTargetClears[CurrentPlayer][count] = NumDropTargetClears[CurrentPlayer][count] % 3;
          }
          challengeFlag *= 2;
        }
        KingsChallengeRunning = KingsChallengeStatus[CurrentPlayer] & KINGS_CHALLENGE_AVAILABLE;
        KingsChallengeStatus[CurrentPlayer] &= ~KINGS_CHALLENGE_AVAILABLE;
        
        if (KingsChallengeRunning & KINGS_CHALLENGE_LEVITATE) LevitateMagnetOnTime = CurrentTime + 4000;

        unsigned long kingsChallengeDuration = ((unsigned long)KingsChallengeBaseTime * 1000)*((unsigned long)numChallengesRunning) + 10000;
        if (DoubleTimers) KingsChallengeEndTime = CurrentTime + kingsChallengeDuration * 2;
        else KingsChallengeEndTime = CurrentTime + kingsChallengeDuration;

        IncreasePlayfieldMultiplier(kingsChallengeDuration + 5000);

        GameModeStage = 0;
        MagnaSaveAvailable = false;
        KingsChallengeBonus = 5000;
        KingsChallengeBonusChangedTime = 0;
        KingsChallengePerfectionBank = CurrentTime%4; 
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.StopAnimation();
        TopperALB.LoopAnimation(TOPPER_FLASH_COLOR_0, 80, 15, 0);
#endif        
      }

      if ( RPU_ReadSingleSwitchState(SW_LEFT_MAGNET_BUTTON) || RPU_ReadSingleSwitchState(SW_RIGHT_MAGNET_BUTTON) ) {
        Audio.StopAllNotifications();
        LockManagementInProgress = false;
        if (KingsChallengeKick==1) {
          RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
        } else if (KingsChallengeKick==2) {
          PlaySoundEffect(SOUND_EFFECT_HORSE_CHUFFING);
          RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 500, true);
        }
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }

      if (CurrentTime > GameModeEndTime) {
        // Eject ball and return to normal play
        if (KingsChallengeKick==1) {
          RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
        } else if (KingsChallengeKick==2) {
          PlaySoundEffect(SOUND_EFFECT_HORSE_CHUFFING);
          RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 500, true);
        }
        KingsChallengeKick = 0;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      break;

  }

  if (TimersPaused) {
    // increaes end time of anything that should be extended
    if (PlayfieldMultiplierExpiration) PlayfieldMultiplierExpiration += (CurrentTime - LastTimeThroughLoop);
    if (KingsChallengeEndTime) KingsChallengeEndTime += (CurrentTime - LastTimeThroughLoop);
  }
  

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowLockLamps();
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowPlayfieldXAndMagnetLamps();
    ShowSpinnerAndPopBumperLamp();
    ShowHeadLamps();
    ShowLaneAndRolloverLamps();
    ShowDropTargetLamps();
    ShowShootAgainLamps();
  }

  // Show spinner progress (when applicable)
  if (LastSpinnerHit != 0 && SpinsTowardsNextGoal[CurrentPlayer] < SpinnerGoal[CurrentPlayer]) {
    if (CurrentTime > (LastSpinnerHit + 3000)) {
      LastSpinnerHit = 0;
      Display_ClearOverride(0xFF);
    } else {
      Display_OverrideScoreDisplay(CurrentPlayer, SpinnerGoal[CurrentPlayer] - SpinsTowardsNextGoal[CurrentPlayer], DISPLAY_OVERRIDE_ANIMATION_CENTER);
    }
  }
    

  if (Display_UpdateDisplays(0xFF, false, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - Display_GetLastTimeScoreChanged()) > 2000)) ? true : false)) {
    Audio.StopSound(SOUND_EFFECT_SCORE_TICK);
    PlaySoundEffect(SOUND_EFFECT_SCORE_TICK);
  }

  if (0 && CurrentTime>(LastTimePromptPlayed+5000)) {
    LastTimePromptPlayed = CurrentTime;
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Tr=%d, Load=%d, BIP=%d, BL=%d\n", CountBallsInTrough(), TotalBallsLoaded, NumberOfBallsInPlay, NumberOfBallsLocked);
      Serial.write(buf);
    }
  }

  // Check to see if ball is in the outhole
  if (CountBallsInTrough() && (CountBallsInTrough() > (TotalBallsLoaded - (NumberOfBallsInPlay + NumberOfBallsLocked)))) {

    if (BallTimeInTrough == 0) {
      // If this is the first time we're seeing too many balls in the trough, we'll wait to make sure
      // everything is settled
      BallTimeInTrough = CurrentTime;
    } else {

      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing or passing through
      if ((CurrentTime - BallTimeInTrough) > 750) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          if (DEBUG_MESSAGES) {
            char buf[128];
            sprintf(buf, "Unqualified and trough=%d, BIP=%d, Lock=%d\n", CountBallsInTrough(), NumberOfBallsInPlay, NumberOfBallsLocked);
            Serial.write(buf);
          }
          RPU_PushToTimedSolenoidStack(SOL_BALL_RAMP_THROWER, 16, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (BallSaveEndTime && CurrentTime < (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
            if (DEBUG_MESSAGES) {
              char buf[128];
              sprintf(buf, "Ball save w/ trough=%d, BIP=%d, Lock=%d\n", CountBallsInTrough(), NumberOfBallsInPlay, NumberOfBallsLocked);
              Serial.write(buf);
            }
            RPU_PushToTimedSolenoidStack(SOL_BALL_RAMP_THROWER, 16, CurrentTime + 100);
            QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);

            RPU_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

            if (DEBUG_MESSAGES) {
              char buf[255];
              sprintf(buf, "Ball Save: BIT=%d, ML=0x%02X, LS=0x%02X, Numlocks=%d, NumBIP=%d\n", CountBallsInTrough(), MachineLocks, PlayerLockStatus[CurrentPlayer], NumberOfBallsLocked, NumberOfBallsInPlay);
              Serial.write(buf);
            }

            // Only 1 ball save if one ball in play
            if (NumberOfBallsInPlay == 1) {
              BallSaveEndTime = CurrentTime + 1000;
            } else {
              if (CurrentTime > BallSaveEndTime) BallSaveEndTime += 1000;
            }

          } else {

            if (DEBUG_MESSAGES) {
              char buf[128];
              sprintf(buf, "Drain: Kick b/c trough=%d, BIP=%d, Lock=%d\n", CountBallsInTrough(), NumberOfBallsInPlay, NumberOfBallsLocked);
              Serial.write(buf);
            }

            NumberOfBallsInPlay -= 1;
            if (DEBUG_MESSAGES) {
              char buf[128];
              sprintf(buf, "Num BIP minus 1 to %d b/c DRAIN\n", NumberOfBallsInPlay);
              Serial.write(buf);
            }
            if (NumberOfBallsInPlay == 0) {
              Display_ClearOverride(0xFF);
              Audio.StopAllAudio();
              MagnaSoundOn = false;
              SetMagnetState(0, false);
              SetMagnetState(1, false);
              returnState = MACHINE_STATE_COUNTDOWN_BONUS;
            }
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;
byte DecrementingBonusCounter;
byte TotalBonus = 0;
byte TotalBonusX = 0;
byte BonusLadderPass = 0;
boolean CountdownBonusHurryUp = false;
boolean RestartCountdownSound = false;

int CountDownDelayTimes[] = {100, 80, 60, 50, 45, 42, 39, 36, 34, 32};

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.StopAnimation();
#endif
    CountdownStartTime = CurrentTime;
    LastCountdownReportTime = CurrentTime;
    ShowBonusXLamps();
    ShowBonusLamps();
    BonusLadderPass = 0;
    RestartCountdownSound = false;
    DecrementingBonusCounter = Bonus[CurrentPlayer];
    TotalBonus = Bonus[CurrentPlayer];
    TotalBonusX = BonusX[CurrentPlayer];
    CountdownBonusHurryUp = false;

    BonusCountDownEndTime = 0xFFFFFFFF;
    if (NumTiltWarnings <= MaxTiltWarnings) PlaySoundEffect(SOUND_EFFECT_COUNTDOWN_BONUS_START);
  }

  unsigned long countdownDelayTime = (unsigned long)(CountDownDelayTimes[BonusLadderPass]);
  if (CountdownBonusHurryUp && countdownDelayTime > ((unsigned long)CountDownDelayTimes[9])) countdownDelayTime = CountDownDelayTimes[9];

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (DecrementingBonusCounter) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        CurrentScores[CurrentPlayer] += 1000;
        if (RestartCountdownSound) {
          PlaySoundEffect(SOUND_EFFECT_COUNTDOWN_BONUS_START + BonusLadderPass);
          RestartCountdownSound = false;
        }
      }

      DecrementingBonusCounter -= 1;
      Bonus[CurrentPlayer] = DecrementingBonusCounter;
      ShowBonusLamps();
      if ( (Bonus[CurrentPlayer] % 10) == 0 ) {
        if (NumTiltWarnings <= MaxTiltWarnings) {
          Audio.StopSound(SOUND_EFFECT_COUNTDOWN_BONUS_START + BonusLadderPass);
          PlaySoundEffect(SOUND_EFFECT_COUNTDOWN_BONUS_END);
          RestartCountdownSound = true;
        }
        BonusLadderPass += 1;
        if (BonusLadderPass >= 9) BonusLadderPass = 9;
      }

    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      if (BonusX[CurrentPlayer] > 1) {
        DecrementingBonusCounter = TotalBonus;
        Bonus[CurrentPlayer] = TotalBonus;
        ShowBonusLamps();
        BonusX[CurrentPlayer] -= 1;
        ShowBonusXLamps();
      } else {
        BonusX[CurrentPlayer] = TotalBonusX;
        Bonus[CurrentPlayer] = TotalBonus;
        BonusCountDownEndTime = CurrentTime + 1000;
      }
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {
    PlaySoundEffect(SOUND_EFFECT_COUNTDOWN_BONUS_END);

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2, true);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    RPU_PushToTimedSolenoidStack(SOL_BELL, 35, CurrentTime, true);
    RPU_PushToTimedSolenoidStack(SOL_BELL, 35, CurrentTime + 300, true);
    RPU_PushToTimedSolenoidStack(SOL_BELL, 35, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    RPU_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      RPU_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      RPU_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        RPU_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      byte upperMask = 0x0F;
      byte lowerMask = 0x30;
      if (RPU_OS_NUM_DIGITS == 7) {
        upperMask = 0x1F;
        lowerMask = 0x60;
      }
      if ( (CurrentTime / 200) % 2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & upperMask);
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | lowerMask);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}




////////////////////////////////////////////////////////////////////////////
//
//  Switch Handling functions
//
////////////////////////////////////////////////////////////////////////////
void HandleLockSwitch(byte lockIndex) {

  // This switch is only "new" if it's not reflected in MachineLocks
  if (MachineLocks & (LOCK_1_ENGAGED << lockIndex)) {
    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Ignoring lock switch %d because ML=0x%04X\n", lockIndex, MachineLocks);
      Serial.write(buf);
    }
    return;
  }

  if (MachineState == MACHINE_STATE_NORMAL_GAMEPLAY) {

    if (GameMode == GAME_MODE_SKILL_SHOT && BallFirstSwitchHitTime==0) {
      RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
    } else if (GameMode == GAME_MODE_UNSTRUCTURED_PLAY) {
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;

      // These are the combat modes
      if (KingsChallengeStatus[CurrentPlayer]&KINGS_CHALLENGE_AVAILABLE) {
        KingsChallengeKick = 1;
        LockManagementInProgress = true;
        SetGameMode(GAME_MODE_KINGS_CHALLENGE_START);
      } else if ( (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK) == 0 && (PlayerLockStatus[CurrentPlayer]&LOCK_1_AVAILABLE) ) {
        // If there are no balls locked, but lock 1 is available, offer single cobat
        if (DEBUG_MESSAGES) {
          Serial.write("HLS: lock 1 avail, so offer single\n");
        }
        LockManagementInProgress = true;
        SetGameMode(GAME_MODE_OFFER_SINGLE_COMBAT);
      } else if ( (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK) == LOCK_1_ENGAGED && (PlayerLockStatus[CurrentPlayer]&LOCK_2_AVAILABLE) ) {
        // If there is 1 ball locked, and lock 2 is available, offer double combat
        if (DEBUG_MESSAGES) {
          Serial.write("HLS: lock 2 avail, so offer double\n");
        }
        LockManagementInProgress = true;
        SetGameMode(GAME_MODE_OFFER_DOUBLE_COMBAT);
      } else if ( (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK) == (LOCK_1_ENGAGED | LOCK_2_ENGAGED) && (PlayerLockStatus[CurrentPlayer]&LOCK_3_AVAILABLE) ) {
        // If the player has 2 balls locked and gets a third, go into triple combat
        if (DEBUG_MESSAGES) {
          Serial.write("HLS: lock 3 avail, so into triple\n");
        }
        LockManagementInProgress = true;
        SetGameMode(GAME_MODE_TRIPLE_COMBAT_START);
      } else {
        if (DEBUG_MESSAGES) {
          Serial.write("HLS: no locks avail, bump\n");
          char buf[128];
          sprintf(buf, "HLS: PlayerLockStatus = 0x%02X\n", PlayerLockStatus[CurrentPlayer]);
          Serial.write(buf);
        }
        PlaySoundEffect(SOUND_EFFECT_SWOOSH);
        RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
      }
    } else if (GameMode == GAME_MODE_SINGLE_COMBAT) {

      // If the player has cleared a bank, they can increase death blow
      byte numBanksCleared = 0;

      // Jackpots and such
      numBanksCleared = ResetAllDropTargets(true);

      if (numBanksCleared) {
        // mors auctus ictu (death blow increased)
        QueueNotification(SOUND_EFFECT_VP_DEATH_BLOW_INCREASED, 10);
        CombatJackpotReady = false;

        unsigned long halfwayTime = ((unsigned long)SingleCombatNumSeconds) * 500;
        if ( (CurrentTime + halfwayTime) > GameModeEndTime ) {
          GameModeEndTime = CurrentTime + halfwayTime;
        }
        LockKickTime[0] = CurrentTime + 2000;
        if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;

        unsigned long jackpotAddition = (CombatJackpot[CurrentPlayer] / 4) * ((unsigned long)numBanksCleared);
        if (jackpotAddition%100) {
          jackpotAddition -= (jackpotAddition%100);
        }
        CombatJackpot[CurrentPlayer] += jackpotAddition;
        JackpotIncreasedTime = CurrentTime;
        if (BallSaveEndTime) {
          BallSaveEndTime += 10000;
        }
      } else {
        if (!ReturnToFightAlreadyPlayed) {
          ReturnToFightAlreadyPlayed = true;
          PlaySoundEffect(SOUND_EFFECT_BOOING_1 + CurrentTime%3);
          QueueNotification(SOUND_EFFECT_VP_RETURN_TO_FIGHT, 8);
        }
        RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
      }


    } else if (GameMode == GAME_MODE_DOUBLE_COMBAT) {
      if (DEBUG_MESSAGES) {
        Serial.write("Double Combat: Handling a lock switch\n");
      }
      // Jackpots and such
      byte numTimedKicks = 0;
      for (byte count = 0; count < 3; count++) {
        if (LockKickTime[count] != 0) numTimedKicks += 1;
      }
      if (SaucerKickTime) numTimedKicks += 1;

      if (!CombatJackpotReady && numTimedKicks == 0) {
        LockKickTime[0] = CurrentTime;
        if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
        if (!ReturnToFightAlreadyPlayed) {
          ReturnToFightAlreadyPlayed = true;
          PlaySoundEffect(SOUND_EFFECT_BOOING_1 + CurrentTime%3);
          QueueNotification(SOUND_EFFECT_VP_RETURN_TO_FIGHT, 8);
        }
      } else {
        if (CombatJackpotReady) {
          // Reset the banks for future jackpots
          ResetAllDropTargets(true);

          if (numTimedKicks == 0) {
            // Award a jackpot for hitting the lock
            QueueNotification(SOUND_EFFECT_VP_JACKPOT, 10);
            Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier, true);
            CombatBankFlags = 0;
            CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP;

            if (GameModeEndTime) {
              // If we're already on a timer, then double combat will end with this
              // jackpot
              RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
              GameModeEndTime = CurrentTime;
            } else {            
              for (byte count = 0; count < 3; count++) {
                if (LockKickTime[count] == 0) {
                  // Tell this lock to kick in 8 seconds
                  if (NumberOfBallsInPlay) {
                    NumberOfBallsInPlay -= 1;
                    if (DEBUG_MESSAGES) {
                      char buf[128];
                      sprintf(buf, "Num BIP minus 1 to %d b/c jackpot\n", NumberOfBallsInPlay);
                      Serial.write(buf);
                    }
                  }
                  LockKickTime[count] = CurrentTime + 8000;
                  PlayLockKickWarningTime = CurrentTime + 7000;
                  break;
                }
              }
            }
          } else {
            // This is a rare case where they got a second jackpot ready
            // while a ball was still waiting to be kicked
            QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 10);
            Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier * 10, true);
            CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP*((unsigned long)10);
            CombatBankFlags = 0;
            boolean kickScheduled = false;
            for (byte count = 0; count < 3; count++) {
              if (LockKickTime[count] == 0 && !kickScheduled) {
                // Tell this lock to kick in 1 second
                LockKickTime[count] = CurrentTime + 1000;
                if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
                kickScheduled = true;
              } else {
                // change kick time
                LockKickTime[count] = CurrentTime + 2000;
              }
            }
          }
          CombatJackpotReady = false;

        } else {
          // award double for second ball in lock
          if (CombatJackpotReady==false && CombatSuperJackpotReady==false) {
            RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
          } else if (SaucerKickTime == 0) {
            // Reset the banks for future jackpots
            ResetAllDropTargets(true);

            QueueNotification(SOUND_EFFECT_VP_DOUBLE_JACKPOT, 10);
            Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier * 2, true);
            CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP*((unsigned long)2);
            CombatBankFlags = 0;
            boolean kickScheduled = false;
            for (byte count = 0; count < 3; count++) {
              if (LockKickTime[count] == 0 && !kickScheduled) {
                // Tell this lock to kick in 1 second
                LockKickTime[count] = CurrentTime + 1000;
                if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
                kickScheduled = true;
              } else {
                // change kick time
                LockKickTime[count] = CurrentTime + 2000;
              }
            }
          } else {
            // Reset the banks for future jackpots
            ResetAllDropTargets(true);

            QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 10);
            Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier * 5, true);
            CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP*((unsigned long)5);
            CombatBankFlags = 0;
            for (byte count = 0; count < 3; count++) {
              if (LockKickTime[count] == 0) {
                // Tell this lock to kick in 1 second
                LockKickTime[count] = CurrentTime + 1000;
                if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
                break;
              }
            }
          }
          CombatJackpotReady = false;
        }
      }

    } else if (GameMode == GAME_MODE_TRIPLE_COMBAT) {
      // Jackpots and such
      if (TripleCombatJackpotsAvailable & TRIPLE_COMBAT_SUPER_JACKPOT) {
        ResetAllDropTargets(true);
        CombatBankFlags = 0;
        TripleCombatJackpotsAvailable = 0;
        if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
          // if saucer, then mega jackpot
          QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 7);
          Display_StartScoreAnimation(PlayfieldMultiplier * 10 * CombatJackpot[CurrentPlayer], true);
          CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_3;
        } else {
          QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 7);
          Display_StartScoreAnimation(PlayfieldMultiplier * 5 * CombatJackpot[CurrentPlayer], true);
          CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_BASE_3;
        }        
        for (byte count = 0; count < 3; count++) {
          if (LockKickTime[count] == 0) {
            // Tell this lock to kick in 1 second
            LockKickTime[count] = CurrentTime + 1000;
            if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
            break;
          }
        }
      } else {
        for (byte count = 0; count < 3; count++) {
          if (LockKickTime[count] == 0) {
            // Tell this lock to kick in 8 seconds
            LockKickTime[count] = CurrentTime + 8000;
            PlayLockKickWarningTime = CurrentTime + 7000;
            if (NumberOfBallsInPlay) NumberOfBallsInPlay -= 1;
            break;
          }
        }
      }
    } else {
      RPU_PushToSolenoidStack(SOL_UPPER_BALL_EJECT, 12, true);
    }


  } else {
    // If we're in attract, or some other non-gameplay mode,
    // we will register the change to MachineLocks, but not do anything
    // about it.
    if (UpperLockSwitchState[0]) MachineLocks |= LOCK_1_ENGAGED;
    else MachineLocks &= ~LOCK_1_ENGAGED;
    if (UpperLockSwitchState[1]) MachineLocks |= LOCK_2_ENGAGED;
    else MachineLocks &= ~LOCK_2_ENGAGED;
    if (UpperLockSwitchState[2]) MachineLocks |= LOCK_3_ENGAGED;
    else MachineLocks &= ~LOCK_3_ENGAGED;
  }
}



int HandleSystemSwitches(int curState, byte switchHit) {
  int returnState = curState;
  switch (switchHit) {
    case SW_SELF_TEST_SWITCH:
      Menus.EnterOperatorMenu();
      break;
    case SW_COIN_1:
    case SW_COIN_2:
    case SW_COIN_3:
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
      break;
    case SW_CREDIT_RESET:
      if (MachineState == MACHINE_STATE_MATCH_MODE) {
        // If the first ball is over, pressing start again resets the game
        if (Credits >= 1 || FreePlayMode) {
          if (!FreePlayMode) {
            Credits -= 1;
            RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
            RPU_SetDisplayCredits(Credits, !FreePlayMode);
          }
          returnState = MACHINE_STATE_INIT_GAMEPLAY;
        }
      } else {
        CreditResetPressStarted = CurrentTime;
      }
      break;
    case SW_OUTHOLE:
      MoveBallFromOutholeToRamp(true);
      break;
    case SW_PLUMB_TILT:
    case SW_ROLL_TILT:
    case SW_PLAYFIELD_TILT:
      if (BallFirstSwitchHitTime) {
        if (IdleMode != IDLE_MODE_BALL_SEARCH && (CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
          LastTiltWarningTime = CurrentTime;
          NumTiltWarnings += 1;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 250, 0, 0);
          TopperALB.PlayAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 250, 0, 0);
          TopperALB.PlayAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 250, 0, 0);
#endif      
          if (NumTiltWarnings==1) {
            OverrideGeneralIllumination(CurrentTime, 150);
          } else if (NumTiltWarnings==2) {
            OverrideGeneralIllumination(CurrentTime, 150);
            OverrideGeneralIllumination(CurrentTime+300, 150);
          }
          if (NumTiltWarnings > MaxTiltWarnings) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.LoopAnimation(TOPPER_PULSE_COLOR_0, 250, 0, 0);
            TopperALB.LoopAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 250, 0, 0);
            TopperALB.LoopAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 250, 0, 0);
            TopperALB.LoopAnimation(GI_0_PULSE_COLOR, 250, 0, 0);
            TopperALB.LoopAnimation(GI_1_PULSE_COLOR, 250, 0, 0);
            TopperALB.LoopAnimation(GI_2_PULSE_COLOR, 250, 0, 0);
#endif      
            SetGeneralIlluminationOn(false);
            RPU_DisableSolenoidStack();
            RPU_SetDisableFlippers(true);
            RPU_TurnOffAllLamps();
            RPU_SetLampState(LAMP_HEAD_TILT, 1);
            Audio.StopAllAudio();
            PlaySoundEffect(SOUND_EFFECT_TILT);
          } else {
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
        }
      } else {
        // Tilt before ball is plunged -- show a timer in ManageGameMode if desired
        if ( CurrentTime > (LastTiltWarningTime + TILT_WARNING_DEBOUNCE_TIME) ) {
          PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
        }
        LastTiltWarningTime = CurrentTime;
      }
      break;
  }

  return returnState;
}


void QualifyKingsChallenge(byte dropTargetBank) {

  byte kingsChallengeFlag = (KINGS_CHALLENGE_1_QUALIFIED<<dropTargetBank);

  if ( (KingsChallengeStatus[CurrentPlayer] & kingsChallengeFlag)==0 ) {
    // Announce challenge
    if (!KCBeginPlayed) {
      QueueNotification(SOUND_EFFECT_VP_KINGS_CHALLENGE_AVAILABLE, 9);
      KCBeginPlayed = true;
    } else {
      QueueNotification(SOUND_EFFECT_VP_KC_QUALIFIED, 9);
    }

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    TopperALB.LoopAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 80, 15, 0);
    TopperALB.LoopAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 80, 15, 0);
#endif

    // Set flag
    KingsChallengeStatus[CurrentPlayer] |= kingsChallengeFlag;
  } else {
    // No need to announce anything because it's already qualified?
  }
  
}


boolean HandleDropTarget(byte bankNum, byte switchHit) {

  if (NumberOfBallsInPlay == 1 && RPU_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
    // we haven't plunged yet, so ignore this hit
    if (DEBUG_MESSAGES) {
      Serial.write("Ball in lane - ignore drop target\n");
    }
    return false;
  }

  DropTargetBank *curBank;
  if (bankNum == 0) curBank = &DropTargetsUL;
  else if (bankNum == 1) curBank = &DropTargetsUR;
  else if (bankNum == 2) curBank = &DropTargetsLL;
  else if (bankNum == 3) curBank = &DropTargetsLR;

  byte result;
  unsigned long numTargetsDown = 0;
  result = curBank->HandleDropTargetHit(switchHit);
  numTargetsDown = (unsigned long)CountBits(result);
  NumDropTargetHits[CurrentPlayer][bankNum] += numTargetsDown;
  boolean joustHit = false;
  boolean perfectionMissed = false;
  CurrentScores[CurrentPlayer] += PlayfieldMultiplier * numTargetsDown * 1000;

  boolean cleared = curBank->CheckIfBankCleared();

  if (numTargetsDown) {

    if (DEBUG_MESSAGES) {
      if (bankNum == 1) {
        char buf[128];
        sprintf(buf, "UR Drops = 0x%02X\n", DropTargetsUR.GetStatus(false));
      }
    }

    if (KingsChallengeRunning&KINGS_CHALLENGE_JOUST && CurrentTime>(LastSwitchHitTime+3000)) {
      // Check if middle target hit
      if (result & 0x02) {
        PlaySoundEffect(SOUND_EFFECT_SINGLE_ANVIL);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
#endif
        if (KingsChallengeBonus<1000000) {
          KingsChallengeBonus += 100000;
          KingsChallengeBonusChangedTime = CurrentTime;
        }
        joustHit = true;
      }
    }

    if (KingsChallengeRunning&KINGS_CHALLENGE_PERFECTION) {
      if (bankNum==KingsChallengePerfectionBank) {
        PlaySoundEffect(SOUND_EFFECT_SINGLE_ANVIL);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
#endif
        if (KingsChallengeBonus<750000) {
          KingsChallengeBonus += 25000;
          KingsChallengeBonusChangedTime = CurrentTime;
          // Reset bank quickly so player can hit more
          DropTargetResetTime[KingsChallengePerfectionBank] = CurrentTime + 1500;
        }
      } else {
        perfectionMissed = true;
      }
    }

    if (KingsChallengeRunning&KINGS_CHALLENGE_LEVITATE) {
      if (bankNum==2) {
        PlaySoundEffect(SOUND_EFFECT_SINGLE_ANVIL);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.LoopAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
        TopperALB.LoopAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
#endif
        if (KingsChallengeBonus<750000) {
          KingsChallengeBonus += 25000;
          KingsChallengeBonusChangedTime = CurrentTime;
          // Reset bank quickly so player can hit more
          DropTargetResetTime[2] = CurrentTime + 1500;
        }
      }
    }

    if (GameMode == GAME_MODE_SKILL_SHOT && BallFirstSwitchHitTime==0) {
      if (bankNum == 0) {
        byte switchIndex = SW_UL_DROP_3 - switchHit;
        if (switchIndex == SkillShotTarget) {
          SkillShotsMade[CurrentPlayer] += 1;
          QueueNotification(SOUND_EFFECT_VP_SUPER_SKILL_SHOT_1 + CurrentTime%2, 10);
          Display_StartScoreAnimation(100000 * ((unsigned long)SkillShotsMade[CurrentPlayer]), true);
        } else {
          if (CurrentBallInPlay==1) QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_MISSED_4-CurrentPlayer, 10);
          else QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_MISSED_1+CurrentTime%5, 10);
        }
      } else {
        if (CurrentBallInPlay==1) QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_MISSED_4-CurrentPlayer, 10);
        else QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_MISSED_1+CurrentTime%5, 10);
      }
      if (DropTargetResetTime[bankNum] == 0) DropTargetResetTime[bankNum] = CurrentTime + 1000;
    } else if (GameMode == GAME_MODE_SINGLE_COMBAT) {
      if (cleared) {

        CombatBankFlags |= (DROP_BANK_UL_FLAG << bankNum);
        boolean jackpotQualified = false;

        if (!CombatJackpotReady) {

          if (SingleCombatLevelCompleted[CurrentPlayer] == 0) {
            jackpotQualified = true;
          } else if (SingleCombatLevelCompleted[CurrentPlayer] == 1) {
            // Upper and lower
            if ( (CombatBankFlags & 0x03) && (CombatBankFlags & 0x0C)) jackpotQualified = true;
          } else {
             if (CombatBankFlags==0x0F) jackpotQualified = true;
          }

          if (jackpotQualified) {
            QueueNotification(SOUND_EFFECT_VP_SAUCER_FOR_DEATHBLOW, 7);
            CombatJackpotReady = true;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD            
            TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
            TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CCW_COLOR_0, 220, 0, 220);
#endif            
            if (DEBUG_MESSAGES) {
              Serial.write("Single combat jackpot ready\n");
            }
          }
        }

        AddToBonus(4);
      }
      PlaySoundEffect(SOUND_EFFECT_SWORD_1 + (CurrentTime) % 7);
      CombatJackpot[CurrentPlayer] += 5000 * numTargetsDown * ((unsigned long)SingleCombatLevelCompleted[CurrentPlayer] + 1);
      JackpotIncreasedTime = CurrentTime;
    } else if (GameMode == GAME_MODE_DOUBLE_COMBAT) {
      if (cleared) {
        CombatBankFlags |= (DROP_BANK_UL_FLAG << bankNum);
        AddToBonus(5);

        if (DoubleCombatLevelCompleted[CurrentPlayer] == 0) {
          // If an upper and lower has been cleared, then jackpot is ready
          if ( (CombatBankFlags & 0x03) && (CombatBankFlags & 0x0C)) {
            if (!CombatJackpotReady) {
              QueueNotification(SOUND_EFFECT_VP_JACKPOT_READY, 7);
            }
            CombatJackpotReady = true;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
            TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
#endif
          }
        } else if (DoubleCombatLevelCompleted[CurrentPlayer] == 1) {
          // If three banks have been cleared
          byte numTargetBanks = CountBits(CombatBankFlags);
          if ( numTargetBanks >= 3 ) {
            if (!CombatJackpotReady) {
              QueueNotification(SOUND_EFFECT_VP_JACKPOT_READY, 7);
            }
            CombatJackpotReady = true;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
            TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
#endif
          }
        } else {
          // If an upper and lower has been cleared, then jackpot is ready
          if ( (CombatBankFlags & 0x03) && (CombatBankFlags & 0x0C)) {
            if (!CombatJackpotReady) {
              QueueNotification(SOUND_EFFECT_VP_JACKPOT_READY, 7);
            }
            CombatJackpotReady = true;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
            TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
            TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 220, 120, 0);
#endif
          }
        }
        
      } else {
        if (DoubleCombatLevelCompleted[CurrentPlayer] > 1) {
          // For level 2 & up, there's a hurry up
          if (DropTargetResetTime[bankNum] == 0) DropTargetResetTime[bankNum] = CurrentTime + ((unsigned long)TimeToResetDrops * 1000);
          DropTargetHurryTime[bankNum] = CurrentTime;
          DropTargetHurryLamp[bankNum] = false;
        }
      }

      PlaySoundEffect(SOUND_EFFECT_SWORD_1 + (CurrentTime) % 7);
      CombatJackpot[CurrentPlayer] += 10000 * numTargetsDown * ((unsigned long)DoubleCombatLevelCompleted[CurrentPlayer] + 1);
      JackpotIncreasedTime = CurrentTime;

    } else if (GameMode == GAME_MODE_TRIPLE_COMBAT) {
      if (cleared) {
        CombatBankFlags |= (DROP_BANK_UL_FLAG << bankNum);
        AddToBonus(6);

        byte numTargets = CountBits(CombatBankFlags);
        if (TripleCombatJackpotsAvailable==0 && numTargets>=(TripleCombatLevelCompleted[CurrentPlayer]+1)) {
          TripleCombatJackpotsAvailable = TRIPLE_COMBAT_ALL_JACKPOTS;
          QueueNotification(SOUND_EFFECT_VP_TRIPLE_JACKPOTS_READY, 7);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.LoopAnimation(LEFT_SPEAKER_LOOP_CW_COLOR_0, 0, 0, 240);
          TopperALB.LoopAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 0, 0, 240);
#endif
        }
      }
      
//      if ( (CombatBankFlags & 0x03) && (CombatBankFlags & 0x0C)) {
//        CombatJackpotReady = true;
//      }
    } else { // not a combat mode or a skill shot

      // Step 3 - handle default behavior
      if (cleared) {
        NumDropTargetClears[CurrentPlayer][bankNum] += 1;

        // Every time we get to a 3rd clear, there are awards
        if (NumDropTargetClears[CurrentPlayer][bankNum]==3) {
          if (bankNum==2) LastChanceStatus[CurrentPlayer] |= LAST_CHANCE_LEFT_QUALIFIED;
          if (bankNum==3) LastChanceStatus[CurrentPlayer] |= LAST_CHANCE_RIGHT_QUALIFIED;
          if (bankNum<2 && NumDropTargetClears[CurrentPlayer][0]==3 && NumDropTargetClears[CurrentPlayer][1]==3) {
            ExtraBallsOrSpecialAvailable[CurrentPlayer] |= EBS_UPPER_EXTRA_BALL_AVAILABLE;
          }

          // Qualify King's Challenges
          QualifyKingsChallenge(bankNum);
        }

        // if they cleared a perfection bank, give them a big bonus (because they reset quickly, it's hard to do)
        if (bankNum==KingsChallengePerfectionBank) {
          if (KingsChallengeBonus<1000000) {
            KingsChallengeBonus += 100000;
          }
        }


        curBank->ResetDropTargets(CurrentTime + 500, true);
        DropTargetResetTime[bankNum] = 0;
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_COMPLETE_1+CurrentTime%3);

        if (RequirePortcullisForLocks) {
          LoopLitToQualifyLock = true;
        } else {
          if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) QualifyNextPlayerLock();
        }
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.PlayAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 0, 255, 0);
        TopperALB.PlayAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 0, 255, 0);
#endif
        // update Magna status for this player
        if (MagnaStatusLeft[CurrentPlayer] < MagnaStatusRight[CurrentPlayer]) {
          MagnaStatusLeft[CurrentPlayer] += 1000;
          if (MagnaStatusLeft[CurrentPlayer] > ((unsigned long)MagnaSaveMaxSeconds*1000)) MagnaStatusLeft[CurrentPlayer] = ((unsigned long)MagnaSaveMaxSeconds*1000);
        } else {
          MagnaStatusRight[CurrentPlayer] += 1000;
          if (MagnaStatusRight[CurrentPlayer] > ((unsigned long)MagnaSaveMaxSeconds*1000)) MagnaStatusRight[CurrentPlayer] = ((unsigned long)MagnaSaveMaxSeconds*1000);
        }

        AddToBonus(3);
      } else {
        if (DropTargetHurryTime[bankNum]) {
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT_1 + CurrentTime % 10);
        } else {
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_HIT_1 + CurrentTime % 10);
        }
        if (DropTargetResetTime[bankNum] == 0) DropTargetResetTime[bankNum] = CurrentTime + ((unsigned long)TimeToResetDrops * 1000);
        DropTargetHurryTime[bankNum] = CurrentTime;
        DropTargetHurryLamp[bankNum] = false;
        if (joustHit) {
          DropTargetResetTime[bankNum] = CurrentTime + 2000;
        }
        if (perfectionMissed) {
          DropTargetResetTime[bankNum] = CurrentTime + 20000; // reset non-perfection banks slowly
//          DropTargetResetTime[KingsChallengePerfectionBank] = CurrentTime + 1000;
        }
      }
    }

    return true;
  }

  return false;
}


void AwardCombo(unsigned long comboFlag) {
  unsigned long comboCallout = 0;

  unsigned long comboStepMultiplier = 1;
  if (ComboStep>3) comboStepMultiplier = 2;
  else if (ComboStep>6) comboStepMultiplier = 3;
  else if (ComboStep>9) comboStepMultiplier = 4;
  
  switch (comboFlag) {
    case COMBO_RIGHT_INLANE_SPINNER:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 5000 * PlayfieldMultiplier * comboStepMultiplier;
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_1;
        Display_StartScoreAnimation(50000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 1");
      }
      break;
    case COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 25000 * PlayfieldMultiplier * comboStepMultiplier;
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_2;
        Display_StartScoreAnimation(100000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 2");
      }
      break;
    case COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP_LOCK:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 100000 * PlayfieldMultiplier * comboStepMultiplier;
      } else {
        DoubleTimers = true;
        comboCallout = SOUND_EFFECT_VP_RELIC_3;
        Display_StartScoreAnimation(500000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 3");
      }
      break;
    case COMBO_RIGHT_INLANE_LOOP:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 15000 * PlayfieldMultiplier * comboStepMultiplier;
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_4;
        Display_StartScoreAnimation(35000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 4");
      }
      break;
    case COMBO_LEFT_INLANE_RAMP:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        RightRampValue[CurrentPlayer] += 25;
        Display_StartScoreAnimation(((unsigned long)RightRampValue[CurrentPlayer])*1000 * PlayfieldMultiplier * comboStepMultiplier, true);
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_5;
        RightRampValue[CurrentPlayer] += 50;
        Display_StartScoreAnimation(((unsigned long)RightRampValue[CurrentPlayer])*1000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 5");
      }
      break;
    case COMBO_LEFT_INLANE_SAUCER:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        Display_StartScoreAnimation(30000 * PlayfieldMultiplier * comboStepMultiplier, true);
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_6;
        Display_StartScoreAnimation(20000 * PlayfieldMultiplier * comboStepMultiplier, true);
        CombatJackpot[CurrentPlayer] += 50000;
        if (DEBUG_MESSAGES) Serial.println("Combo 6");
      }
      break;
    case COMBO_SAUCER_RAMP:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        RightRampValue[CurrentPlayer] += 25;
        Display_StartScoreAnimation(((unsigned long)RightRampValue[CurrentPlayer])*1000 * PlayfieldMultiplier * comboStepMultiplier, true);
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_8;
        RightRampValue[CurrentPlayer] += 75;
        Display_StartScoreAnimation(((unsigned long)RightRampValue[CurrentPlayer])*1000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DoubleTimers) IncreasePlayfieldMultiplier(30000);
        else IncreasePlayfieldMultiplier(15000);
        if (DEBUG_MESSAGES) Serial.println("Combo 8");
      }
      break;
    case COMBO_SAUCER_SAUCER:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        Display_StartScoreAnimation(40000 * PlayfieldMultiplier * comboStepMultiplier, true);
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_7;
        Display_StartScoreAnimation(30000 * PlayfieldMultiplier * comboStepMultiplier, true);
        CombatJackpot[CurrentPlayer] += 100000;
        if (DEBUG_MESSAGES) Serial.println("Combo 7");
      }
      break;
    case COMBO_LOOP_LEFT_INLANE:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 20000 * PlayfieldMultiplier * comboStepMultiplier;
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_9;
        LastChanceAvailable = true;
        Display_StartScoreAnimation(50000 * PlayfieldMultiplier * comboStepMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 9");
      }
      break;
    case COMBO_LEFT_RIGHT_PASS:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 20000 * comboStepMultiplier * PlayfieldMultiplier;
      } else {
        Display_StartScoreAnimation(20000 * comboStepMultiplier * PlayfieldMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo left right");
      }
      break;
    case COMBO_RIGHT_LEFT_PASS:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 20000 * comboStepMultiplier * PlayfieldMultiplier;
      } else {
        Display_StartScoreAnimation(20000 * comboStepMultiplier * PlayfieldMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo right left");
      }
      break;
    case COMBO_LEFT_INLANE_RAMP_LOCK:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        CurrentScores[CurrentPlayer] += 100000 * comboStepMultiplier * PlayfieldMultiplier;
      } else {
        comboCallout = SOUND_EFFECT_VP_RELIC_3 * comboStepMultiplier * PlayfieldMultiplier;
        Display_StartScoreAnimation(500000, true);
        if (DEBUG_MESSAGES) Serial.println("Combo 3");
      }
      break;
    case COMBO_ALL:
      if (CombosAchieved[CurrentPlayer]&comboFlag) {
        Display_StartScoreAnimation(750000 * PlayfieldMultiplier, true);
      } else {
        DoubleTimers = true;
        LastChanceAvailable = true;
        comboCallout = SOUND_EFFECT_VP_RELIC_ALL;
        CombosAchieved[CurrentPlayer] |= 0xFFFFFFFF;
        Display_StartScoreAnimation(500000 * PlayfieldMultiplier, true);
        if (DEBUG_MESSAGES) Serial.println("Combo all");
      }
      break;
  }
  
  CombosAchieved[CurrentPlayer] |= comboFlag;
  if (comboCallout) QueueNotification(comboCallout, 6);
}


unsigned long ComboReleaseTime = 2500;

void UpdateComboStatus() {

  if (LeftComboLastHitTime && CurrentTime > (LeftComboLastHitTime+ComboReleaseTime)) {
    LeftComboLastHitTime = 0;
  }

  if (RightComboLastHitTime && CurrentTime > (RightComboLastHitTime+ComboReleaseTime)) {
    RightComboLastHitTime = 0;
  }

  if (LeftComboLastHitTime==0) {
    if (RightRampLitFromCombo) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.StopAnimation(GI_0_FLICKER_COLOR);
#endif      
      RightRampLitFromCombo = false;
    }
    SaucerLitFromCombo = false;
    RightInlaneLitFromCombo = false;    
  }

  if (RightComboLastHitTime==0) {
    SpinnerLitFromCombo = false;
    LoopLitFromCombo = false;
    LeftInlaneLitFromCombo = false;
    UpperLeftRolloverLitFromCombo = false;
  }

  if (RightComboLastHitTime==0 && LeftComboLastHitTime==0) {
    LockLitFromCombo = false;
    if (ComboStep) {
      ComboStep = 0;
      LastComboSwitch = SWITCH_STACK_EMPTY;
    }
  }
  
}

byte ChurchBellSequence[] = {51, 80, 81, 82, 82, 81, 51, 80, 51, 81, 80, 82, 82, 81, 51, 80, 82, 82, 82, 82};

void AdvanceCombos(byte switchHit) {

  if (LeftComboLastHitTime==0 && RightComboLastHitTime==0 && switchHit!=SW_LEFT_INSIDE_ROLLOVER && switchHit!=SW_RIGHT_INSIDE_ROLLOVER) {
    // If no combo is started and none
    // are being started, then return immediately
    return;
  }

  // Some switches will immediately turn off a combo
  if (  switchHit==SW_LEFT_SLING || switchHit==SW_RIGHT_SLING || switchHit==SW_POP_BUMPER /*||
        switchHit==SW_LL_DROP_1 || switchHit==SW_LL_DROP_2 || switchHit==SW_LL_DROP_3 ||
        switchHit==SW_LR_DROP_1 || switchHit==SW_LR_DROP_2 || switchHit==SW_LR_DROP_3 */) {
    LeftComboLastHitTime = 0;
    RightComboLastHitTime = 0;
    ComboStep = 0;
    return;
  }

  byte lastStep = ComboStep;

  if (switchHit==SW_LEFT_INSIDE_ROLLOVER) {
    ComboStep += 1;
    if (LeftComboLastHitTime==0) {
      // Start the left combo tracker
      LeftComboLastHitTime = CurrentTime;
      ComboReleaseTime = ((unsigned long)BaseComboTime * 100);
    }

    if (LastComboSwitch==SW_LOOP) {
      // Award COMBO_LOOP_LEFT_INLANE
      AwardCombo(COMBO_LOOP_LEFT_INLANE);
    } else if (LastComboSwitch==SW_RIGHT_INSIDE_ROLLOVER) {
      AwardCombo(COMBO_RIGHT_LEFT_PASS);
    }
    LastComboSwitch = SW_LEFT_INSIDE_ROLLOVER;
    RightRampLitFromCombo = true;
    SaucerLitFromCombo = true;
    RightInlaneLitFromCombo = true;
    
  } else if (switchHit==SW_RIGHT_INSIDE_ROLLOVER) {
    ComboStep += 1;
    if (RightComboLastHitTime==0) {
      // Start the left combo tracker
      RightComboLastHitTime = CurrentTime;
      ComboReleaseTime = ((unsigned long)BaseComboTime * 100);
    }
    
    if (LastComboSwitch==SW_LEFT_INSIDE_ROLLOVER) {
      AwardCombo(COMBO_LEFT_RIGHT_PASS);
    }
    LastComboSwitch = SW_RIGHT_INSIDE_ROLLOVER;
    SpinnerLitFromCombo = true;
    LoopLitFromCombo = true;
    LeftInlaneLitFromCombo = true;

  } else if (switchHit==SW_LOOP) {
    if (RightComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime*100 + 500);
      ComboStep += 1;
      AwardCombo(COMBO_RIGHT_INLANE_LOOP);
      LastComboSwitch = SW_LOOP;
      LeftInlaneLitFromCombo = true;
    }
  } else if (switchHit==SW_SPINNER) {
    if (RightComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime*100 + 1000);
      if (LastComboSwitch==SW_RIGHT_INSIDE_ROLLOVER) {
        ComboStep += 1;
        AwardCombo(COMBO_RIGHT_INLANE_SPINNER);
        LastComboSwitch = SW_SPINNER;
        UpperLeftRolloverLitFromCombo = true;
      }
      RightComboLastHitTime = CurrentTime;
    }
  } else if (switchHit==SW_LEFT_RAMP_ROLLOVER) {
    if (RightComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime * 100);
      if (LastComboSwitch==SW_SPINNER) {
        ComboStep += 1;
        AwardCombo(COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP);
      }
      LastComboSwitch = SW_LEFT_RAMP_ROLLOVER;
      LockLitFromCombo = true;
    }
  } else if (switchHit==SW_LOCK_3) {
    if (RightComboLastHitTime || LeftComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime * 100);
      if (LastComboSwitch==SW_LEFT_RAMP_ROLLOVER) {        
        ComboStep += 1;
        AwardCombo(COMBO_RIGHT_INLANE_SPINNER_LEFT_RAMP_LOCK);
      } else if (LastComboSwitch==SW_RIGHT_RAMP_ROLLUNDER) {
        ComboStep += 1;
        AwardCombo(COMBO_LEFT_INLANE_RAMP_LOCK);
      } else if (LastComboSwitch==SW_LOCK_3) {
        ComboStep += 1;        
      }
      LastComboSwitch = SW_LOCK_3;
    }
  } else if (switchHit==SW_SAUCER) {
    if (LeftComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime * 200);
      if (LastComboSwitch==SW_LEFT_INSIDE_ROLLOVER) {
        ComboStep += 1;
        AwardCombo(COMBO_LEFT_INLANE_SAUCER);
      } else if (LastComboSwitch==SW_SAUCER) {
        ComboStep += 1;
        AwardCombo(COMBO_SAUCER_SAUCER);
      }
      LastComboSwitch = SW_SAUCER;
    }
  } else if (switchHit==SW_RIGHT_RAMP_ROLLUNDER) {
    if (LeftComboLastHitTime) {
      ComboReleaseTime = ((unsigned long)BaseComboTime * 100);
      if (LastComboSwitch==SW_LEFT_INSIDE_ROLLOVER) {
        ComboStep += 1;
        AwardCombo(COMBO_LEFT_INLANE_RAMP);
      } else if (LastComboSwitch==SW_SAUCER) {
        ComboStep += 1;
        AwardCombo(COMBO_SAUCER_RAMP);
      }
      LastComboSwitch = SW_RIGHT_RAMP_ROLLUNDER;
      LockLitFromCombo = true;
    }
  }



  // Play the appropriate church bell and update timing
  if (ComboStep > lastStep) {
    // Keep combo timing refreshed
    if (LeftComboLastHitTime) LeftComboLastHitTime = CurrentTime;
    if (RightComboLastHitTime) RightComboLastHitTime = CurrentTime;

    // Play combo advance sound
    if (ComboStep < 21) PlaySoundEffect(ChurchBellSequence[ComboStep-1]);
    if (ComboStep==CombosUntilOmnia) AwardCombo(COMBO_ALL);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
    if (RightRampLitFromCombo) TopperALB.PlayAnimation(GI_0_FLICKER_COLOR, 200, 0, 200, 2500);

    switch(ComboStep) {
      case 1:
      case 5:
      case 9:
      case 13:      
        TopperALB.PlayAnimation(BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE);
        break;
      case 2:
      case 6:
      case 10:
      case 14:
        TopperALB.PlayAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 0, 0, 200);
        break;
      case 3:
      case 7:
      case 11:
      case 15:
        TopperALB.PlayAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 0, 0, 200);
        break;
      case 4:
      case 8:
      case 12:
      case 16:
        TopperALB.PlayAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 200, 200, 0);
        break;
      default:
        TopperALB.PlayAnimation(BOTH_SPEAKERS_TWO_COLOR_0_1_PULSE);
        TopperALB.PlayAnimation(TOPPER_PULSE_COLOR_0, 200, 200, 0);
        break;
    }
#endif    
  }

}


void QualifyNextPlayerLock() {

  byte playerLocks = PlayerLockStatus[CurrentPlayer];

  // if there are no more locks to qualify -- return
  if ( (playerLocks & LOCKS_ENGAGED_MASK) == LOCKS_ENGAGED_MASK ) return;

  LoopLitToQualifyLock = false;

  for (byte count = 0; count < 3; count++) {
    if ( (playerLocks & (LOCK_1_ENGAGED << count)) == 0 && (playerLocks & (LOCK_1_AVAILABLE << count)) == 0) {
      PlayerLockStatus[CurrentPlayer] |= (LOCK_1_AVAILABLE << count);
      PlaySoundEffect(SOUND_EFFECT_PORTCULLIS);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.PlayAnimation(LEFT_SPEAKER_PULSE_COLOR_0, 0, 255, 0);        
      TopperALB.PlayAnimation(RIGHT_SPEAKER_PULSE_COLOR_0, 0, 255, 0);        
#endif
      return;
    }
  }

  PlaySoundEffect(SOUND_EFFECT_DOOR_SLAM);

}

void RemoveTopQualifiedFlag() {

  byte playerLocks = PlayerLockStatus[CurrentPlayer];

  // if there are no more locks to qualify -- return
  if ( (playerLocks & LOCKS_AVAILABLE_MASK) == 0 ) return;

  for (byte count = 0; count < 3; count++) {
    if ( (playerLocks & (LOCK_3_AVAILABLE >> count)) ) {
      PlayerLockStatus[CurrentPlayer] &= ~(playerLocks & (LOCK_3_AVAILABLE >> count));
      break;
    }
  }
}



boolean AwardTripleJackpotIfAvailable(byte switchHit) {
  if (TripleCombatJackpotsAvailable==0) return false;

  boolean jackpotAwarded = false;
  if (switchHit==SW_SPINNER && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_SPINNER_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_SPINNER_JACKPOT;
    jackpotAwarded = true;
  } else if (switchHit==SW_RIGHT_RAMP_ROLLUNDER && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_MIDDLE_RAMP_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_MIDDLE_RAMP_JACKPOT;
    jackpotAwarded = true;
  } else if (switchHit==SW_LOOP && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_LOOP_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_LOOP_JACKPOT;
    jackpotAwarded = true;
  } else if (switchHit==SW_SAUCER && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_SAUCER_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_SAUCER_JACKPOT;
    jackpotAwarded = true;
  } else if (switchHit==SW_LOCK_3 && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_LOCK_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_LOCK_JACKPOT;
    jackpotAwarded = true;
  } else if (switchHit==SW_LEFT_RAMP_ROLLOVER && (TripleCombatJackpotsAvailable&TRIPLE_COMBAT_UPPER_RAMP_JACKPOT)) {
    TripleCombatJackpotsAvailable &= ~TRIPLE_COMBAT_UPPER_RAMP_JACKPOT;
    jackpotAwarded = true;
  }

  if (jackpotAwarded) {
    QueueNotification(SOUND_EFFECT_VP_JACKPOT_ALT_0 + (CurrentTime%5), 7);
    Display_StartScoreAnimation(PlayfieldMultiplier * CombatJackpot[CurrentPlayer], true);
    CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP;
    RPU_PushToSolenoidStack(SOL_BELL, 20, true);

    if (TripleCombatJackpotsAvailable==0 && GameMode==GAME_MODE_TRIPLE_COMBAT) {
      QueueNotification(SOUND_EFFECT_VP_TRIPLE_SUPER_READY, 7);
      TripleCombatJackpotsAvailable |= TRIPLE_COMBAT_SUPER_JACKPOT;
    }
    
    return true;
  }

  return false;
}


void HandleSaucer() {

  AdvanceCombos(SW_SAUCER);

  byte numTimedKicks = 0;
  for (byte count = 0; count < 3; count++) {
    if (LockKickTime[count] != 0) numTimedKicks += 1;
  }

  if (BonusXCollectAvailable) {
    BonusXCollectAvailable = false;
    BonusXCollectReminder = 0;
    NumBonusXCollectReminders = 0;
    IncreaseBonusX();
  }

  boolean jackpotAwarded = false;

  if (GameMode == GAME_MODE_SINGLE_COMBAT) {
    if (CombatJackpotReady) {
      SetGameMode(GAME_MODE_SINGLE_COMBAT_WON);
    } else {
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
      QueueNotification(SOUND_EFFECT_VP_SINGLE_OPPONENT_RALLIES, 8);
      RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 1000, true);
      ResetAllDropTargets();
    }
  } else if (GameMode == GAME_MODE_DOUBLE_COMBAT) {

    if (CombatJackpotReady) {
      if (numTimedKicks == 0) {
        // Award a jackpot for hitting the lock
        QueueNotification(SOUND_EFFECT_VP_JACKPOT, 10);
        if (CombatBankFlags==0x0F) Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier * 2, true);
        else Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier, true);
        CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP;
        CombatBankFlags = 0;
        SaucerKickTime = CurrentTime + 8000;
        PlaySaucerKickWarningTime = CurrentTime + 7000;
      } else {
        // This is a rare case where they got a second jackpot ready
        // while a ball was still waiting to be kicked
        QueueNotification(SOUND_EFFECT_VP_MEGA_JACKPOT, 10);
        ResetAllDropTargets(true);
        Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * PlayfieldMultiplier * 10, true);
        CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP*((unsigned long)10);
        CombatBankFlags = 0;
        for (byte count = 0; count < 3; count++) {
          if (LockKickTime[count] != 0) {
            // Tell this lock to kick in 1 second (update the kick time)
            LockKickTime[count] = CurrentTime + 1000;
          }
        }
        SaucerKickTime = CurrentTime + 2000;
      }
      CombatJackpotReady = false;
    } else if (numTimedKicks) {
      if (CombatSuperJackpotReady) {
        // There's a ball waiting in the lock, so we can get a super jackpot
        QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 10);
        Display_StartScoreAnimation(CombatJackpot[CurrentPlayer] * 5 * PlayfieldMultiplier, true);
        CombatJackpot[CurrentPlayer] += COMBAT_JACKPOT_STEP*((unsigned long)5);
        SaucerKickTime = CurrentTime + 4000;
        PlaySaucerKickWarningTime = CurrentTime + 3000;
        CombatSuperJackpotReady = false;
      } else {
        // Double combat jackpot was used, so we'll just hold
        SaucerKickTime = CurrentTime + 8000;
        PlaySaucerKickWarningTime = CurrentTime + 7000;
      }
    } else {
      RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 1000, true);
    }
  } else if (GameMode == GAME_MODE_TRIPLE_COMBAT) {
    jackpotAwarded = AwardTripleJackpotIfAvailable(SW_SAUCER);
    SaucerKickTime = CurrentTime + 8000;
    PlaySaucerKickWarningTime = CurrentTime + 7000;
  } else if (KingsChallengeStatus[CurrentPlayer]&KINGS_CHALLENGE_AVAILABLE) {
    PlaySoundEffect(SOUND_EFFECT_DOOR_SLAM);
    KingsChallengeKick = 2;
    SetGameMode(GAME_MODE_KINGS_CHALLENGE_START);
  } else {
    PlaySoundEffect(SOUND_EFFECT_HORSE_CHUFFING);
    RPU_PushToTimedSolenoidStack(SOL_SAUCER, 16, CurrentTime + 1000, true);
  }

  if (!jackpotAwarded) CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
}


void AwardSpinnerGoal() {
  SpinsTowardsNextGoal[CurrentPlayer] = 0;
  SpinnerGoal[CurrentPlayer] += 10;
  BonusXCollectReminder = CurrentTime + 15000;
  BonusXCollectAvailable = true;
  BonusXCollectAvailableStart = CurrentTime;
  PlaySoundEffect(SOUND_EFFECT_THREE_DINGS);
  NumBonusXCollectReminders = 0;
}


void RemindBonusXCollect() {
  BonusXCollectReminder = CurrentTime + 10000;
  BonusXCollectAvailableStart = CurrentTime;
  PlaySoundEffect(SOUND_EFFECT_THREE_DINGS);
  NumBonusXCollectReminders += 1;
  if (NumBonusXCollectReminders==6) {
    NumBonusXCollectReminders = 0;
    QueueNotification(SOUND_EFFECT_VP_BONUS_X_COLLECT_INSTRUCTIONS, 7);
  }
}



void ConvertOneLockToQualified() {
  byte engaged = (PlayerLockStatus[CurrentPlayer] / 2) & LOCKS_ENGAGED_MASK;

  byte requalified = LOCK_1_AVAILABLE;
  if (PlayerLockStatus[CurrentPlayer] & LOCK_3_ENGAGED) requalified = LOCK_3_AVAILABLE;
  else if (PlayerLockStatus[CurrentPlayer] & LOCK_2_ENGAGED) requalified = LOCK_2_AVAILABLE;

  PlayerLockStatus[CurrentPlayer] = (PlayerLockStatus[CurrentPlayer] & (~LOCKS_ENGAGED_MASK)) | engaged | requalified;
}



void HandleGamePlaySwitches(byte switchHit) {

  if (GameMode==GAME_MODE_WAIT_FOR_BALL_TO_RETURN) return;

  // Saucer hits are called by debouncer

  switch (switchHit) {

    case SW_LEFT_SLING:
      AdvanceCombos(switchHit);
//      if (CurrentTime < (BallSearchSolenoidFireTime[BALL_SEARCH_LEFT_SLING_INDEX] + 150)) break;
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      LastSwitchHitTime = CurrentTime;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.PlayAnimation(TOPPER_LEFT_FLASH_COLOR_0, 0, 50, 150);
#endif
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;
      
    case SW_RIGHT_SLING:
      AdvanceCombos(switchHit);
//      if (CurrentTime < (BallSearchSolenoidFireTime[BALL_SEARCH_RIGHT_SLING_INDEX] + 150)) break;
      CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10;
      PlaySoundEffect(SOUND_EFFECT_SLING_SHOT);
      LastSwitchHitTime = CurrentTime;
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.PlayAnimation(TOPPER_RIGHT_FLASH_COLOR_0, 0, 50, 150);
#endif
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_UL_DROP_1:
    case SW_UL_DROP_2:
    case SW_UL_DROP_3:
      AdvanceCombos(switchHit);
      if (HandleDropTarget(0, switchHit)) {
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_UR_DROP_1:
    case SW_UR_DROP_2:
    case SW_UR_DROP_3:
      AdvanceCombos(switchHit);
      if (HandleDropTarget(1, switchHit)) {
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_LL_DROP_1:
    case SW_LL_DROP_2:
    case SW_LL_DROP_3:
      AdvanceCombos(switchHit);
      if (HandleDropTarget(2, switchHit)) {
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_LR_DROP_1:
    case SW_LR_DROP_2:
    case SW_LR_DROP_3:
      AdvanceCombos(switchHit);
      if (HandleDropTarget(3, switchHit)) {
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      }
      break;

    case SW_LOOP:
      AdvanceCombos(switchHit);
      if (!AwardTripleJackpotIfAvailable(SW_LOOP)) {
        if (LoopLitToQualifyLock && (GameMode==GAME_MODE_UNSTRUCTURED_PLAY)) {
          QualifyNextPlayerLock();
          Display_StartScoreAnimation(50000, true);
        } else {
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
          PlaySoundEffect(SOUND_EFFECT_SWOOSH);
        }
      }
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.PlayAnimation(TOPPER_LOOP_COLOR_0, 0, 250, 0);
#endif      
      LastLoopHitTime = CurrentTime;
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_POP_BUMPER:
      AdvanceCombos(switchHit);
      if (KingsChallengeRunning & KINGS_CHALLENGE_MELEE) {
        if (KingsChallengeBonus<500000) {
          PlaySoundEffect(SOUND_EFFECT_SINGLE_ANVIL);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.LoopAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
          TopperALB.LoopAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 80, 15, 0);
#endif
          KingsChallengeBonus += KingsChallengeBonus/5;
          KingsChallengeBonus -= KingsChallengeBonus%50;
          KingsChallengeBonusChangedTime = CurrentTime;
        } else {
          PlaySoundEffect(SOUND_EFFECT_SWORD_1 + CurrentTime%7);
        }
      } else if (BallFirstSwitchHitTime) {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
        TopperALB.PlayAnimation(LEFT_SPEAKER_FLASH_COLOR_0, 75, 0, 75);
        TopperALB.PlayAnimation(RIGHT_SPEAKER_FLASH_COLOR_0, 75, 0, 75);
#endif      
        LastPopBumperHit = CurrentTime;
        CurrentScores[CurrentPlayer] += 500 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_POP_BUMPER);
      }
      LastSwitchHitTime = CurrentTime;
      break;

    case SW_SPINNER:
      AdvanceCombos(switchHit);
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
      TopperALB.PlayAnimation(LEFT_SPEAKER_LOOP_CCW_COLOR_0, 75, 20, 175);
      TopperALB.PlayAnimation(RIGHT_SPEAKER_LOOP_CW_COLOR_0, 75, 20, 175);
#endif      
      AwardTripleJackpotIfAvailable(SW_SPINNER);
      if (SpinnerStatus) {
        CurrentScores[CurrentPlayer] += 2500 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_SPINNER_LIT_2);
        SpinnerLitUntil = CurrentTime + 1000;
        if (!BonusXCollectAvailable) SpinsTowardsNextGoal[CurrentPlayer] += 5;
      } else {
        CurrentScores[CurrentPlayer] += 100 * PlayfieldMultiplier;
        PlaySoundEffect(SOUND_EFFECT_SPINNER_UNLIT);
        if (!BonusXCollectAvailable) SpinsTowardsNextGoal[CurrentPlayer] += 1;
      }

      if (!BonusXCollectAvailable && (SpinsTowardsNextGoal[CurrentPlayer] > SpinnerGoal[CurrentPlayer])) {
        AwardSpinnerGoal();
      }

      LastSwitchHitTime = CurrentTime;
      LastSpinnerHit = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_RIGHT_INSIDE_ROLLOVER:
      if (CurrentTime > (LastRightInlane + 250)) {
        AdvanceCombos(switchHit);
        SpinnerLitUntil = CurrentTime + 3000;
        SpinnerStatus = 1;
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
        if (LastTimeRightMagnetOn==0) {
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2000;
          AddToBonus(2);
        } else {
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10000;
          AddToBonus(5);
        }
        PlaySoundEffect(SOUND_EFFECT_CHURCH_BELL_1);
        LastRightInlane = CurrentTime;
      }
      LastSwitchHitTime = CurrentTime;
      break;

    case SW_LEFT_INSIDE_ROLLOVER:
      if (CurrentTime > (LastLeftInlane + 250)) {
        AdvanceCombos(switchHit);
        LastSwitchHitTime = CurrentTime;
        if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
        if (LastTimeLeftMagnetOn==0) {
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 2000;
          AddToBonus(2);
        } else {
          CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 10000;
          AddToBonus(5);
        }
        PlaySoundEffect(SOUND_EFFECT_CHURCH_BELL_1);
        LastLeftInlane = CurrentTime;
      }
      LastSwitchHitTime = CurrentTime;
      break;

    case SW_LOCK_3:
      AdvanceCombos(switchHit);
      if (GameMode==GAME_MODE_SKILL_SHOT) {
        SkillShotsMade[CurrentPlayer] += 1;
        QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT, 10);
        Display_StartScoreAnimation(50000 * ((unsigned long)SkillShotsMade[CurrentPlayer]), true);
      }
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      if (!AwardTripleJackpotIfAvailable(SW_LOCK_3)) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 1000;
      }
      break;

    case SW_SAUCER:
//      HandleSaucer();
//      LastSwitchHitTime = CurrentTime;
//      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      break;

    case SW_LEFT_OUTLANE:
      if (CurrentTime > (LastLeftOutlane + 250)) {
        LastLeftOutlane = CurrentTime;

        if ( (LastChanceAvailable) && (LastChanceStatus[CurrentPlayer]&LAST_CHANCE_LEFT_QUALIFIED) && (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK)) {
          ConvertOneLockToQualified();
          for (byte count = 0; count < 3; count++) {
            if (LockKickTime[count] == 0) {
              LockKickTime[count] = CurrentTime + 100;
              break;
            }
          }
          NumberOfBallsInPlay += 1;
          if (NumberOfBallsLocked) NumberOfBallsLocked -= 1;
          LastChanceStatus[CurrentPlayer] &= ~(LAST_CHANCE_LEFT_QUALIFIED);
          PlaySoundEffect(SOUND_EFFECT_FANFARE_2);
  
          if (DEBUG_MESSAGES) {
            char buf[255];
            sprintf(buf, "Last Chance: BIT=%d, ML=0x%02X, LS=0x%02X, Numlocks=%d, NumBIP=%d\n", CountBallsInTrough(), MachineLocks, PlayerLockStatus[CurrentPlayer], NumberOfBallsLocked, NumberOfBallsInPlay);
            Serial.write(buf);
          }
  
        } else {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.PlayAnimation(TOPPER_RIGHT_TO_LEFT_COLOR_0, 250, 0, 0);
#endif
          if (BallSaveEndTime) BallSaveEndTime += 3000;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
          if (LastTimeLeftMagnetOn) Audio.PlaySoundCardWhenPossible(22, CurrentTime, 0, 1000, 10);
          LastSwitchHitTime = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
        }
        AddToBonus(2);
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
      }
      break;

    case SW_RIGHT_OUTLANE:
      if (CurrentTime > (LastRightOutlane + 250)) {
        LastRightOutlane = CurrentTime;

        if ( (CurrentBallInPlay==BallsPerGame) && (LastChanceStatus[CurrentPlayer]&LAST_CHANCE_RIGHT_QUALIFIED) && (PlayerLockStatus[CurrentPlayer]&LOCKS_ENGAGED_MASK)) {
          ConvertOneLockToQualified();
          for (byte count = 0; count < 3; count++) {
            if (LockKickTime[count] == 0) {
              LockKickTime[count] = CurrentTime + 100;
              break;
            }
          }
//          NumberOfBallsInPlay += 1;
          LastChanceStatus[CurrentPlayer] &= ~(LAST_CHANCE_RIGHT_QUALIFIED);
          PlaySoundEffect(SOUND_EFFECT_FANFARE_2);
          if (DEBUG_MESSAGES) {
            char buf[255];
            sprintf(buf, "Last Chance: BIT=%d, ML=0x%02X, LS=0x%02X, Numlocks=%d, NumBIP=%d\n", CountBallsInTrough(), MachineLocks, PlayerLockStatus[CurrentPlayer], NumberOfBallsLocked, NumberOfBallsInPlay);
            Serial.write(buf);
          }
        } else {
#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
          TopperALB.PlayAnimation(TOPPER_LEFT_TO_RIGHT_COLOR_0, 250, 0, 0);
#endif
          if (BallSaveEndTime) BallSaveEndTime += 3000;
          PlaySoundEffect(SOUND_EFFECT_OUTLANE_UNLIT);
          if (LastTimeRightMagnetOn) Audio.PlaySoundCardWhenPossible(22, CurrentTime, 0, 1000, 10);
          LastSwitchHitTime = CurrentTime;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
        }
        AddToBonus(2);
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
      }
      break;

    case SW_RIGHT_RAMP_ROLLUNDER:
      AdvanceCombos(switchHit);
      AwardTripleJackpotIfAvailable(SW_RIGHT_RAMP_ROLLUNDER);
      break;

    case SW_LEFT_RAMP_ROLLOVER:
      AdvanceCombos(switchHit);
      if (ExtraBallsOrSpecialAvailable[CurrentPlayer] & EBS_UPPER_EXTRA_BALL_AVAILABLE) {
        ExtraBallsOrSpecialAvailable[CurrentPlayer] &= ~EBS_UPPER_EXTRA_BALL_AVAILABLE;
        AwardExtraBall();
      }
      LastSwitchHitTime = CurrentTime;
      if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
      
      if (!AwardTripleJackpotIfAvailable(SW_LEFT_RAMP_ROLLOVER)) {
        CurrentScores[CurrentPlayer] += PlayfieldMultiplier * 5000;
      }
      break;
  }

}


int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    Display_ClearOverride(0xFF);
    Display_UpdateDisplays(0xFF, true);
    returnState = CountdownBonus(curStateChanged);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);

    if (SamePlayerShootsAgain) {
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 10);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          RPU_SetDisplay(count, CurrentScores[count], true, 2, true);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  UpdateLockStatus();
  MoveBallFromOutholeToRamp();

  byte switchHit;
  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;

  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    returnState = HandleSystemSwitches(curState, switchHit);
    if (NumTiltWarnings <= MaxTiltWarnings) HandleGamePlaySwitches(switchHit);
  }

  if (CreditResetPressStarted) {
    if (CurrentBallInPlay < 2) {
      // If we haven't finished the first ball, we can add players
      AddPlayer();
      if (DEBUG_MESSAGES) {
        Serial.write("Start game button pressed\n\r");
      }
      CreditResetPressStarted = 0;
    } else {
      if (RPU_ReadSingleSwitchState(SW_CREDIT_RESET)) {
        if (TimeRequiredToResetGame != 99 && (CurrentTime - CreditResetPressStarted) >= ((unsigned long)TimeRequiredToResetGame * 1000)) {
          // If the first ball is over, pressing start again resets the game
          if (Credits >= 1 || FreePlayMode) {
            if (!FreePlayMode) {
              Credits -= 1;
              RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
              RPU_SetDisplayCredits(Credits, !FreePlayMode);
            }
            returnState = MACHINE_STATE_INIT_GAMEPLAY;
            CreditResetPressStarted = 0;
          }
        }
      } else {
        CreditResetPressStarted = 0;
      }
    }
  }

  if (lastBallFirstSwitchHitTime == 0 && BallFirstSwitchHitTime != 0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)BallSaveNumSeconds) * 1000;
  }
  if (CurrentTime > (BallSaveEndTime + BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > RPU_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= RPU_OS_MAX_DISPLAY_SCORE;
    if (!TournamentScoring) AddSpecialCredit();
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    Display_SetLastTimeScoreChanged(CurrentTime);
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (ExtraBallsCollected<MaxExtraBallsPerBall) {
            AwardExtraBall();
          }
        }
      }
    }

  }

  return returnState;
}


unsigned long LastLEDUpdateTime = 0;
byte LEDPhase = 0;
unsigned long NumLoops = 0;
unsigned long LastLoopReportTime = 0;

void loop() {

  /*
    if (DEBUG_MESSAGES) {
      NumLoops += 1;
      if (CurrentTime>(LastLoopReportTime+1000)) {
        LastLoopReportTime = CurrentTime;
        char buf[128];
        sprintf(buf, "Loop running at %lu Hz\n", NumLoops);
        Serial.write(buf);
        NumLoops = 0;
      }
    }
  */

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (Menus.OperatorMenusActive()) {
    RunOperatorMenu();
  } else {
    if (MachineState < 0) {
      newMachineState = 0;
    } else if (MachineState == MACHINE_STATE_ATTRACT) {
      newMachineState = RunAttractMode(MachineState, MachineStateChanged);
    } else if (MachineState == MACHINE_STATE_DIAGNOSTICS) {
      newMachineState = RunDiagnosticsMode(MachineState, MachineStateChanged);
    } else {
      newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
    }
  }
  
  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  //  RPU_ApplyFlashToLamps(CurrentTime);
  //  RPU_UpdateTimedSolenoidStack(CurrentTime);
  //  RPU_UpdateTimedSoundStack(CurrentTime);
  RPU_Update(CurrentTime);
  Audio.Update(CurrentTime);

#ifdef RPU_OS_USE_ACCESSORY_LAMP_BOARD
  if (CurrentTime >  (ALBWatchdogResetLastSent + ALB_WATCHDOG_TIMER_DURATION/2)) {
    TopperALB.WatchdogTimerReset();
    ALBWatchdogResetLastSent = CurrentTime;
  }
#endif

  /*
    if (LastLEDUpdateTime == 0 || (CurrentTime - LastLEDUpdateTime) > 250) {
      LastLEDUpdateTime = CurrentTime;
      RPU_SetBoardLEDs((LEDPhase % 8) == 1 || (LEDPhase % 8) == 3, (LEDPhase % 8) == 5 || (LEDPhase % 8) == 7);
      LEDPhase += 1;
    }
  */
}

  
