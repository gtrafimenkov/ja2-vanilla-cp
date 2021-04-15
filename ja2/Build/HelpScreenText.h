#ifndef HELP_SCREEN_TEXT__H_
#define HELP_SCREEN_TEXT__H_

#define HELPSCREEN_RECORD_SIZE 80 * 8

// enum for the help text paragrphs
enum {
  HLP_TXT_CONSTANT_SUBTITLE,  // 0
  HLP_TXT_CONSTANT_FOOTER,
  HLP_TXT_LAPTOP_TITLE,
  HLP_TXT_LAPTOP_BUTTON_1,
  HLP_TXT_LAPTOP_OVERVIEW_P1,
  HLP_TXT_LAPTOP_OVERVIEW_P2,
  HLP_TXT_LAPTOP_BUTTON_2,
  HLP_TXT_LAPTOP_EMAIL_P1,
  HLP_TXT_LAPTOP_BUTTON_3,
  HLP_TXT_LAPTOP_WEB_P1,

  HLP_TXT_LAPTOP_BUTTON_4,  // 10
  HLP_TXT_LAPTOP_FILES_P1,
  HLP_TXT_LAPTOP_BUTTON_5,
  HLP_TXT_LAPTOP_HISTORY_P1,
  HLP_TXT_LAPTOP_BUTTON_6,
  HLP_TXT_LAPTOP_PERSONNEL_P1,

  HLP_TXT_LAPTOP_BUTTON_7,
  HLP_TXT_FINANCES_P1,
  HLP_TXT_FINANCES_P2,

  HLP_TXT_LAPTOP_BUTTON_8,
  HLP_TXT_MERC_STATS_P1,
  HLP_TXT_MERC_STATS_P2,
  HLP_TXT_MERC_STATS_P3,
  HLP_TXT_MERC_STATS_P4,
  HLP_TXT_MERC_STATS_P5,
  HLP_TXT_MERC_STATS_P6,
  HLP_TXT_MERC_STATS_P7,
  HLP_TXT_MERC_STATS_P8,
  HLP_TXT_MERC_STATS_P9,
  HLP_TXT_MERC_STATS_P10,
  HLP_TXT_MERC_STATS_P11,
  HLP_TXT_MERC_STATS_P12,
  HLP_TXT_MERC_STATS_P13,
  HLP_TXT_MERC_STATS_P14,
  HLP_TXT_MERC_STATS_P15,

  // mapscreen no one hired yet
  HLP_TXT_MPSCRN_NO_1_HIRED_YET_TITLE,
  HLP_TXT_MPSCRN_NO_1_HIRED_YET_P1,  // 20
  HLP_TXT_MPSCRN_NO_1_HIRED_YET_P2,

  // mapscreen not in arulco yet
  HLP_TXT_MPSCRN_NOT_IN_ARULCO_TITLE,

  HLP_TXT_MPSCRN_NOT_IN_ARULCO_P1,
  HLP_TXT_MPSCRN_NOT_IN_ARULCO_P2,
  HLP_TXT_MPSCRN_NOT_IN_ARULCO_P3,

  HLP_TXT_WELCOM_TO_ARULCO_TITLE,
  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_1,
  HLP_TXT_WELCOM_TO_ARULCO_OVERVIEW_P1,
  HLP_TXT_WELCOM_TO_ARULCO_OVERVIEW_P2,

  HLP_TXT_WELCOM_TO_ARULCO_OVERVIEW_P3,  // 30
  HLP_TXT_WELCOM_TO_ARULCO_OVERVIEW_P4,
  HLP_TXT_WELCOM_TO_ARULCO_OVERVIEW_P5,
  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_2,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P1,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P2,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P3,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P4,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P5,
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P6,

  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P7,  // 40
  HLP_TXT_WELCOM_TO_ARULCO_ASSNMNT_P8,
  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_3,
  HLP_TXT_WELCOM_TO_ARULCO_DSTINATION_P1,
  HLP_TXT_WELCOM_TO_ARULCO_DSTINATION_P2,
  HLP_TXT_WELCOM_TO_ARULCO_DSTINATION_P3,
  HLP_TXT_WELCOM_TO_ARULCO_DSTINATION_P4,
  HLP_TXT_WELCOM_TO_ARULCO_DSTINATION_P5,
  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_4,
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P1,

  HLP_TXT_WELCOM_TO_ARULCO_MAP_P2,  // 50
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P3,
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P4,
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P5,
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P6,
  HLP_TXT_WELCOM_TO_ARULCO_MAP_P7,
  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_5,
  HLP_TXT_WELCOM_TO_ARULCO_MILITIA_P1,
  HLP_TXT_WELCOM_TO_ARULCO_MILITIA_P2,

  HLP_TXT_WELCOM_TO_ARULCO_MILITIA_P3,  // 60

  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_6,
  HLP_TXT_WELCOM_TO_ARULCO_AIRSPACE_P1,
  HLP_TXT_WELCOM_TO_ARULCO_AIRSPACE_P2,

  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_7,
  HLP_TXT_WELCOM_TO_ARULCO_ITEMS_P1,

  HLP_TXT_WELCOM_TO_ARULCO_BUTTON_8,
  HLP_TXT_WELCOM_TO_ARULCO_KEYBOARD_P1,
  HLP_TXT_WELCOM_TO_ARULCO_KEYBOARD_P2,
  HLP_TXT_WELCOM_TO_ARULCO_KEYBOARD_P3,
  HLP_TXT_WELCOM_TO_ARULCO_KEYBOARD_P4,

  HLP_TXT_TACTICAL_TITLE,
  HLP_TXT_TACTICAL_BUTTON_1,
  HLP_TXT_TACTICAL_OVERVIEW_P1,
  HLP_TXT_TACTICAL_OVERVIEW_P2,
  HLP_TXT_TACTICAL_OVERVIEW_P3,
  HLP_TXT_TACTICAL_OVERVIEW_P4,

  HLP_TXT_TACTICAL_BUTTON_2,
  HLP_TXT_TACTICAL_MOVEMENT_P1,
  HLP_TXT_TACTICAL_MOVEMENT_P2,
  HLP_TXT_TACTICAL_MOVEMENT_P3,
  HLP_TXT_TACTICAL_MOVEMENT_P4,

  HLP_TXT_TACTICAL_BUTTON_3,
  HLP_TXT_TACTICAL_SIGHT_P1,
  HLP_TXT_TACTICAL_SIGHT_P2,
  HLP_TXT_TACTICAL_SIGHT_P3,
  HLP_TXT_TACTICAL_SIGHT_P4,

  HLP_TXT_TACTICAL_BUTTON_4,
  HLP_TXT_TACTICAL_ATTACKING_P1,
  HLP_TXT_TACTICAL_ATTACKING_P2,
  HLP_TXT_TACTICAL_ATTACKING_P3,

  HLP_TXT_TACTICAL_BUTTON_5,

  HLP_TXT_TACTICAL_ITEMS_P1,
  HLP_TXT_TACTICAL_ITEMS_P2,
  HLP_TXT_TACTICAL_ITEMS_P3,
  HLP_TXT_TACTICAL_ITEMS_P4,
  HLP_TXT_TACTICAL_BUTTON_6,

  HLP_TXT_TACTICAL_KEYBOARD_P1,
  HLP_TXT_TACTICAL_KEYBOARD_P2,
  HLP_TXT_TACTICAL_KEYBOARD_P3,
  HLP_TXT_TACTICAL_KEYBOARD_P4,
  HLP_TXT_TACTICAL_KEYBOARD_P5,
  HLP_TXT_TACTICAL_KEYBOARD_P6,
  HLP_TXT_TACTICAL_KEYBOARD_P7,
  HLP_TXT_TACTICAL_KEYBOARD_P8,

  HLP_TXT_SECTOR_INVTRY_TITLE,
  //	HLP_TXT_SECTOR_INVTRY_BUTTON_1,
  HLP_TXT_SECTOR_INVTRY_OVERVIEW_P1,
  HLP_TXT_SECTOR_INVTRY_OVERVIEW_P2,

  //	HLP_TXT_,
};

#endif
