/**
 * @file group.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions for handling window groups.
 *
 */

#include "jwm.h"
#include "group.h"
#include "client.h"
#include "icon.h"
#include "error.h"
#include "misc.h"
#include "settings.h"

#include <regex.h>

/** What part of the window to match. */
typedef unsigned int MatchType;
#define MATCH_NAME   0  /**< Match the window name. */
#define MATCH_CLASS  1  /**< Match the window class. */

/** List of match patterns for a group. */
typedef struct PatternListType {
   char *pattern;
   MatchType match;
   struct PatternListType *next;
} PatternListType;

/** List of options for a group. */
typedef struct OptionListType {
   OptionType option;
   char *str;
   union {
      unsigned u;
      int s;
   } value;
   struct OptionListType *next;
} OptionListType;

/** List of groups. */
typedef struct GroupType {
   PatternListType *patterns;
   OptionListType *options;
   struct GroupType *next;
} GroupType;

static GroupType *groups = NULL;

static void ReleasePatternList(PatternListType *lp);
static void ReleaseOptionList(OptionListType *lp);
static void AddPattern(PatternListType **lp, const char *pattern,
                       MatchType match);
static void ApplyGroup(const GroupType *gp, ClientNode *np);

/** Determine if expression matches pattern. */
static char Match(const char *pattern, const char *expression);

/** Destroy group data. */
void DestroyGroups(void)
{
   GroupType *gp;
   while(groups) {
      gp = groups->next;
      ReleasePatternList(groups->patterns);
      ReleaseOptionList(groups->options);
      Release(groups);
      groups = gp;
   }
}

/** Release a group pattern list. */
void ReleasePatternList(PatternListType *lp)
{
   PatternListType *tp;
   while(lp) {
      tp = lp->next;
      Release(lp->pattern);
      Release(lp);
      lp = tp;
   }
}

/** Release a group option list. */
void ReleaseOptionList(OptionListType *lp)
{
   OptionListType *tp;
   while(lp) {
      tp = lp->next;
      if(lp->str) {
         Release(lp->str);
      }
      Release(lp);
      lp = tp;
   }
}

/** Create an empty group. */
GroupType *CreateGroup(void)
{
   GroupType *tp;
   tp = new GroupType;
   tp->patterns = NULL;
   tp->options = NULL;
   tp->next = groups;
   groups = tp;
   return tp;
}

/** Add a window class to a group. */
void AddGroupClass(GroupType *gp, const char *pattern)
{
   Assert(gp);
   if(JLIKELY(pattern)) {
      AddPattern(&gp->patterns, pattern, MATCH_CLASS);
   } else {
      Warning(_("invalid group class"));
   }
}

/** Add a window name to a group. */
void AddGroupName(GroupType *gp, const char *pattern)
{
   Assert(gp);
   if(JLIKELY(pattern)) {
      AddPattern(&gp->patterns, pattern, MATCH_NAME);
   } else {
      Warning(_("invalid group name"));
   }
}

/** Add a pattern to a pattern list. */
void AddPattern(PatternListType **lp, const char *pattern, MatchType match)
{
   PatternListType *tp;
   Assert(lp);
   Assert(pattern);
   tp = new PatternListType;
   tp->next = *lp;
   *lp = tp;
   tp->pattern = CopyString(pattern);
   tp->match = match;
}

/** Add an option to a group. */
void AddGroupOption(GroupType *gp, OptionType option)
{
   OptionListType *lp;
   lp = new OptionListType;
   lp->option = option;
   lp->str = NULL;
   lp->next = gp->options;
   gp->options = lp;
}

/** Add an option (with a string) to a group. */
void AddGroupOptionString(GroupType *gp, OptionType option,
                          const char *value)
{
   OptionListType *lp;
   Assert(value);
   lp = new OptionListType;
   lp->option = option;
   lp->str = CopyString(value);
   lp->next = gp->options;
   gp->options = lp;
}

/** Add an option (with an unsigned integer) to a group. */
void AddGroupOptionUnsigned(GroupType *gp, OptionType option,
                            unsigned value)
{
   OptionListType *lp;
   Assert(value);
   lp = new OptionListType;
   lp->option = option;
   lp->str = NULL;
   lp->value.u = value;
   lp->next = gp->options;
   gp->options = lp;
}

/** Add an option (with a signed integer) to a group. */
void AddGroupOptionSigned(GroupType *gp, OptionType option,
                          int value)
{
   OptionListType *lp;
   Assert(value);
   lp = new OptionListType;
   lp->option = option;
   lp->str = NULL;
   lp->value.s = value;
   lp->next = gp->options;
   gp->options = lp;
}

/** Apply groups to a client. */
void ApplyGroups(ClientNode *np)
{
   PatternListType *lp;
   GroupType *gp;
   char hasClass;
   char hasName;
   char matchesClass;
   char matchesName;

   Assert(np);
   for(gp = groups; gp; gp = gp->next) {
      hasClass = 0;
      hasName = 0;
      matchesClass = 0;
      matchesName = 0;
      for(lp = gp->patterns; lp; lp = lp->next) {
         if(lp->match == MATCH_CLASS) {
            if(Match(lp->pattern, np->getClassName())) {
               matchesClass = 1;
            }
            hasClass = 1;
         } else if(lp->match == MATCH_NAME) {
            if(Match(lp->pattern, np->getInstanceName())) {
               matchesName = 1;
            }
            hasName = 1;
         } else {
            Debug("invalid match in ApplyGroups: %d", lp->match);
         }
      }
      if(hasName == matchesName && hasClass == matchesClass) {
         ApplyGroup(gp, np);
      }
   }

}

/** Apply a group to a client. */
void ApplyGroup(const GroupType *gp, ClientNode *np)
{
   OptionListType *lp;
   char noPager = 0;
   char noList = 0;

   Assert(gp);
   Assert(np);
   for(lp = gp->options; lp; lp = lp->next) {
      switch(lp->option) {
      case OPTION_STICKY:
         np->getState()->status |= STAT_STICKY;
         break;
      case OPTION_NOLIST:
         np->getState()->status |= STAT_NOLIST;
         noList = 1;
         break;
      case OPTION_ILIST:
         np->getState()->status |= STAT_ILIST;
         if(!noList) {
            np->getState()->status &= ~STAT_NOLIST;
         }
         break;
      case OPTION_NOPAGER:
         np->getState()->status |= STAT_NOPAGER;
         noPager = 1;
         break;
      case OPTION_IPAGER:
         np->getState()->status |= STAT_IPAGER;
         if(!noPager) {
            np->getState()->status &= ~STAT_NOPAGER;
         }
         break;
      case OPTION_BORDER:
         np->getState()->border |= BORDER_OUTLINE;
         break;
      case OPTION_NOBORDER:
         np->getState()->border &= ~BORDER_OUTLINE;
         break;
      case OPTION_TITLE:
         np->getState()->border |= BORDER_TITLE;
         break;
      case OPTION_NOTITLE:
         np->getState()->border &= ~BORDER_TITLE;
         np->getState()->border &= ~BORDER_SHADE;
         break;
      case OPTION_LAYER:
         np->getState()->layer = lp->value.u;
         break;
      case OPTION_DESKTOP:
         if(JLIKELY(lp->value.u >= 1 && lp->value.u <= settings.desktopCount)) {
            np->getState()->desktop = lp->value.u - 1;
         } else {
            Warning(_("invalid group desktop: %d"), lp->value.u);
         }
         break;
      case OPTION_ICON:
         DestroyIcon(np->getIcon());
         np->setIcon(LoadNamedIcon(lp->str, 1, 1));
         break;
      case OPTION_PIGNORE:
         np->getState()->status |= STAT_PIGNORE;
         break;
      case OPTION_IIGNORE:
         np->getState()->status |= STAT_IIGNORE;
         break;
      case OPTION_MAXIMIZED:
         np->getState()->maxFlags |= MAX_HORIZ | MAX_VERT;
         break;
      case OPTION_MINIMIZED:
         np->getState()->status |= STAT_MINIMIZED;
         break;
      case OPTION_SHADED:
         np->getState()->status |= STAT_SHADED;
         break;
      case OPTION_OPACITY:
         np->getState()->opacity = lp->value.u;
         np->getState()->status |= STAT_OPACITY;
         break;
      case OPTION_MAX_V:
         np->getState()->border &= ~BORDER_MAX_H;
         break;
      case OPTION_MAX_H:
         np->getState()->border &= ~BORDER_MAX_V;
         break;
      case OPTION_NOFOCUS:
         np->getState()->status |= STAT_NOFOCUS;
         break;
      case OPTION_NOSHADE:
         np->getState()->border &= ~BORDER_SHADE;
         break;
      case OPTION_NOMIN:
         np->getState()->border &= ~BORDER_MIN;
         break;
      case OPTION_NOMAX:
         np->getState()->border &= ~BORDER_MAX;
         break;
      case OPTION_NOCLOSE:
         np->getState()->border &= ~BORDER_CLOSE;
         break;
      case OPTION_NOMOVE:
         np->getState()->border &= ~BORDER_MOVE;
         break;
      case OPTION_NORESIZE:
         np->getState()->border &= ~BORDER_RESIZE;
         break;
      case OPTION_CENTERED:
         np->getState()->status |= STAT_CENTERED;
         break;
      case OPTION_TILED:
         np->getState()->status |= STAT_TILED;
         break;
      case OPTION_NOTURGENT:
         np->getState()->status |= STAT_NOTURGENT;
         break;
      case OPTION_CONSTRAIN:
         np->getState()->border |= BORDER_CONSTRAIN;
         break;
      case OPTION_FULLSCREEN:
         np->getState()->status |= STAT_FULLSCREEN;
         break;
      case OPTION_NOFULLSCREEN:
         np->getState()->border &= ~BORDER_FULLSCREEN;
         break;
      case OPTION_DRAG:
         np->getState()->status |= STAT_DRAG;
         break;
      case OPTION_FIXED:
         np->getState()->status |= STAT_FIXED;
         break;
      case OPTION_AEROSNAP:
         np->getState()->status |= STAT_AEROSNAP;
         break;
      case OPTION_NODRAG:
         np->getState()->status |= STAT_NODRAG;
         break;
      case OPTION_X:
         if(lp->value.s < 0) {
            int north, south, east, west;
            Border::GetBorderSize(np->getState(), &north, &south, &east, &west);
            np->setX(rootWidth + lp->value.s - np->getWidth() - east - west);
         } else {
            np->setX(lp->value.s);
         }
         np->getState()->status |= STAT_POSITION;
         break;
      case OPTION_Y:
         if(lp->value.s < 0) {
            int north, south, east, west;
            Border::GetBorderSize(np->getState(), &north, &south, &east, &west);
            np->setY(rootHeight + lp->value.s - np->getHeight() - north - south);
         } else {
           np->setY(lp->value.s);
         }
         np->getState()->status |= STAT_POSITION;
         break;
      case OPTION_WIDTH:
         np->setWidth(Max(1, lp->value.u));
         break;
      case OPTION_HEIGHT:
         np->setHeight(Max(1, lp->value.u));
         break;
      default:
         Debug("invalid option: %d", lp->option);
         break;
      }
   }

}

/** Determine if expression matches pattern. */
char Match(const char *pattern, const char *expression)
{

   regex_t re;
   regmatch_t rm;
   int rc;

   if(!pattern && !expression) {
      return 1;
   } else if(!pattern || !expression) {
      return 0;
   }

   if(regcomp(&re, pattern, REG_EXTENDED) != 0) {
      return 0;
   }

   rc = regexec(&re, expression, 0, &rm, 0);

   regfree(&re);

   return rc == 0 ? 1 : 0;

}

