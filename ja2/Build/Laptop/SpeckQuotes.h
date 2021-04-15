#ifndef _SPECK_QUOTES_H_
#define _SPECK_QUOTES_H_

// Enum,s for all of specks quotes
enum {
  SPECK_QUOTE_FIRST_TIME_IN_0,  // 0
  SPECK_QUOTE_FIRST_TIME_IN_1,
  SPECK_QUOTE_FIRST_TIME_IN_2,
  SPECK_QUOTE_FIRST_TIME_IN_3,
  SPECK_QUOTE_FIRST_TIME_IN_4,
  SPECK_QUOTE_FIRST_TIME_IN_5,
  SPECK_QUOTE_FIRST_TIME_IN_6,
  SPECK_QUOTE_FIRST_TIME_IN_7,
  SPECK_QUOTE_FIRST_TIME_IN_8,
  SPECK_QUOTE_THANK_PLAYER_FOR_OPENING_ACCOUNT,

  SPECK_QUOTE_ALTERNATE_OPENING_1_TOUGH_START,  // 10
  SPECK_QUOTE_ALTERNATE_OPENING_2_BUSINESS_BAD,
  SPECK_QUOTE_ALTERNATE_OPENING_3_BUSINESS_GOOD,
  SPECK_QUOTE_ALTERNATE_OPENING_4_TRYING_TO_RECRUIT,
  SPECK_QUOTE_ALTERNATE_OPENING_5_PLAYER_OWES_SPECK_ACCOUNT_SUSPENDED,
  SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_1,
  SPECK_QUOTE_ALTERNATE_OPENING_6_PLAYER_OWES_SPECK_ALMOST_BANKRUPT_2,
  SPECK_QUOTE_ALTERNATE_OPENING_7_PLAYER_ACCOUNT_OK,
  SPECK_QUOTE_ALTERNATE_OPENING_8_MADE_PARTIAL_PAYMENT,
  SPECK_QUOTE_ALTERNATE_OPENING_9_FIRST_VISIT_SINCE_SERVER_WENT_DOWN,

  SPECK_QUOTE_ALTERNATE_OPENING_10_GENERIC_OPENING,  // 20
  SPECK_QUOTE_ALTERNATE_OPENING_10_TAG_FOR_20,
  SPECK_QUOTE_ALTERNATE_OPENING_11_NEW_MERCS_AVAILABLE,
  SPECK_QUOTE_ALTERNATE_OPENING_12_PLAYERS_LOST_MERCS,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_OWES_MONEY,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_DEBT,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_PLAYER_CLEARED_OVERDUE_ACCOUNT,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_FIRST_MERC_DIES,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_BIFF_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_HAYWIRE_IS_DEAD,

  SPECK_QUOTE_ALTERNATE_OPENING_TAG_GASKET_IS_DEAD,  // 30
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_RAZOR_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_ALIVE,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_IS_DEAD_BIFF_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_FLO_MARRIED_A_COUSIN_BIFF_IS_ALIVE,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_GUMPY_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_LARRY_RELAPSED,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_COUGER_IS_DEAD,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_NUMB_IS_DEAD,

  SPECK_QUOTE_ALTERNATE_OPENING_TAG_BUBBA_IS_DEAD,  // 40
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_1,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_2,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_3,
  SPECK_QUOTE_ALTERNATE_OPENING_TAG_ON_AFTER_OTHER_TAGS_4,
  SPECK_QUOTE_PLAYER_MAKES_FULL_PAYMENT,
  SPECK_QUOTE_PLAYER_MAKES_PARTIAL_PAYMENT,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BIFF,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_HAYWIRE,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GASKET,

  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_RAZOR,  // 50
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_FLO,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_GUMPY,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_LARRY,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_COUGER,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_NUMB,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_SPECK_SELLS_BUBBA,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_TAG_GETTING_MORE_MERCS,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_1,
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_2,

  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_3,  // 60
  SPECK_QUOTE_PLAYER_NOT_DOING_ANYTHING_AIM_SLANDER_4,
  SPECK_QUOTE_BIFF_UNAVALIABLE,
  SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_LARRY,
  SPECK_QUOTE_PLAYERS_HIRES_BIFF_SPECK_PLUGS_FLO,
  SPECK_QUOTE_PLAYERS_HIRES_HAYWIRE_SPECK_PLUGS_RAZOR,
  SPECK_QUOTE_PLAYERS_HIRES_RAZOR_SPECK_PLUGS_HAYWIRE,
  SPECK_QUOTE_PLAYERS_HIRES_FLO_SPECK_PLUGS_BIFF,
  SPECK_QUOTE_PLAYERS_HIRES_LARRY_SPECK_PLUGS_BIFF,
  SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_1,

  SPECK_QUOTE_GENERIC_THANKS_FOR_HIRING_MERCS_2,  // 70
  SPECK_QUOTE_PLAYER_TRIES_TO_HIRE_ALREADY_HIRED_MERC,
  SPECK_QUOTE_GOOD_BYE_1,
  SPECK_QUOTE_GOOD_BYE_2,
  SPECK_QUOTE_GOOD_BYE_3,
  SPECK_QUOTE_GOOD_BYE_TAG_PLAYER_OWES_MONEY,

  /*
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,

          SPECK_QUOTE_,						//80
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
          SPECK_QUOTE_,
  */
};

#endif
