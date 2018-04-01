/*
 * reasons.cpp
 *
 * Part of "Dark Tower: All Hail" LARP music engine.
 *
 * Generated automatically by EmotionProcessor.py
 * from Reasons.csv
 *
 * !!! DO NOT EDIT !!!
 *
 * Generated at %(currentTime)s
 */
#include "emotions.h"

{%int %(rName)s_ID;}

void prepare_globals(const InfluenceTable &table)
{
{%    %(rName)s_ID = table.find("%(rName)s");}
}

// End of reasons.cpp
