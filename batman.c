/* batman -- automatic theme toggle for Windows
   Copyright (C) 2024 Martín Aguilar

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <HTTP://www.gnu.org/licenses/>. */

/* This program works by getting the state of the "Night Light", applying
   the corresponding theme.

   By ik7swordking@gmail.com, Martín Aguilar. */

#include <windows.h>
#include <stdio.h>

/* Time in milliseconds to wait for state detection. */
#define INTERVAL 1500

/* Registry key path for the Night Light state. */
#define NIGHT_LIGHT_REGISTRY_KEY "SOFTWARE\\Microsoft\\Windows\\\
CurrentVersion\\CloudStore\\Store\\DefaultAccount\\Current\\\
default$windows.data.bluelightreduction.bluelightreductionstate\\\
windows.data.bluelightreduction.bluelightreductionstate"

#define SYSTEM_THEME_REGISTRY_KEY "SOFTWARE\\Microsoft\\Windows\\\
CurrentVersion\\Themes\\Personalize"

/* Non-nil if Windows "Night Light" feature is enabled.  */
static BOOL
get_night_light_enabled_state ()
{
  byte value[256];
  DWORD buffer_size = sizeof (value);

  LSTATUS result = RegGetValueA (HKEY_CURRENT_USER,
                                 NIGHT_LIGHT_REGISTRY_KEY,
                                 "Data",
                                 RRF_RT_REG_BINARY,
                                 NULL, value, &buffer_size);

  if (result == ERROR_SUCCESS)
    return value[23] == 0x10 && value[24] == 0x00;

  return FALSE;
}

/* Sets the system theme. Non-nil if it was successful. */
BOOL
set_light_theme_state (BOOL state)
{
  DWORD buffer_size = sizeof (state);

  HKEY theme_key;
  LSTATUS result = RegOpenKeyExA(HKEY_CURRENT_USER,
                                 SYSTEM_THEME_REGISTRY_KEY,
                                 0, KEY_WRITE, &theme_key);
  if (result == ERROR_SUCCESS)
    {
      LSTATUS system_result = RegSetValueExA(theme_key,
                                             "SystemUsesLightTheme",
                                             0, REG_DWORD,
                                             (const BYTE *) &state, buffer_size);

      LSTATUS apps_result = RegSetValueExA(theme_key,
                                           "AppsUseLightTheme",
                                           0, REG_DWORD,
                                           (const BYTE *) &state, buffer_size);

      if (system_result == ERROR_SUCCESS
          && apps_result == ERROR_SUCCESS)
        return TRUE;
    }

  return FALSE;
}

/* Non-nil if program is currently running. */
BOOL is_program_running = TRUE;

/* Internal control handler for shutting down gracefully. */
BOOL WINAPI
control_handler (DWORD control_type)
{ 
  switch (control_type)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
      printf("batman: shutdown!\n");
      is_program_running = FALSE;
      return TRUE;
    }
  return FALSE;
}

int
main (void)
{
  if (!SetConsoleCtrlHandler (control_handler, TRUE))
    {
      printf ("batman: error while setting control handler.");
      return 1;
    }

  BOOL is_already_dark = !get_night_light_enabled_state ();
  set_light_theme_state (is_already_dark);
  
  while (is_program_running)
    {
      BOOL new_night_light_state = get_night_light_enabled_state ();

      if ((new_night_light_state && !is_already_dark)
          || (!new_night_light_state && is_already_dark))
        {
          if (set_light_theme_state (!new_night_light_state))
            {
              printf("batman: set %s theme\n", new_night_light_state ? "dark" : "light");
              is_already_dark = !is_already_dark;
            }
          else
            printf("batman: could not set %s theme.\n", new_night_light_state ? "dark" : "light");
        }

      Sleep(INTERVAL);
    }

  return 0;
}
