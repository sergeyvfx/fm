/**
 * ${project-name} - a GNU/Linux console-based file manager
 *
 * Signals' handling stuff
 *
 * Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
 * Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
 *
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#include "signals.h"
#include "hook.h"

#include <signal.h>

static struct
{
  /* Number of signal */
  int signum;

  /* Default handler of signal */
  sighandler_t def_handler;

  /* Name of hook to call */
  wchar_t *hook;
} signals[] = {
  {SIGINT,  NULL, L"signal-int-hook"},
  {SIGHUP,  NULL, L"signal-hup-hook"},
  {SIGTERM, NULL, L"signal-term-hook"},
  {-1, NULL, NULL}
};

static void
signal_handler (int __signum)
{
  int i = 0;
  sighandler_t handler = NULL;
  wchar_t *hook = NULL;

  /* Get name of hook and default handler */
  while (signals[i].signum >= 0)
    {
      /* Descriptor is found */
      if (signals[i].signum == __signum)
        {
          hook = signals[i].hook;
          handler = signals[i].def_handler;
          break;
        }
      ++i;
    }

  if (hook)
    {
      /*
       * TODO: Need to send some context's information
       */
      if (hook_call (hook, NULL) == HOOK_BREAK)
        {
          /* Some handler doesn't want signal to be handled */
          return;
        }
    }

  /* Call default handler */
  if (handler)
    {
      handler (__signum);
    }
}

/**
 * Set hooks to signals
 */
void
signals_hook (void)
{
  int i = 0;

  while (signals[i].signum >= 0)
    {
      signals[i].def_handler = signal (signals[i].signum, signal_handler);
      ++i;
    }
}

