/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"
#include "wterminal.h"

#include "wposix_internal.h"
#include "crt_posix.h"		// _get_osfhandle


int ioctl(int fd, int op, int* data)
{
	const HANDLE h = HANDLE_from_intptr(_get_osfhandle(fd));

	switch(op)
	{
	case TIOCMGET:
		/* TIOCM_* mapped directly to MS_*_ON */
		GetCommModemStatus(h, (DWORD*)data);
		break;

	case TIOCMBIS:
		/* only RTS supported */
		if(*data & TIOCM_RTS)
			EscapeCommFunction(h, SETRTS);
		else
			EscapeCommFunction(h, CLRRTS);
		break;

	case TIOCMIWAIT:
		static DWORD mask;
		DWORD new_mask = 0;
		if(*data & TIOCM_CD)
			new_mask |= EV_RLSD;
		if(*data & TIOCM_CTS)
			new_mask |= EV_CTS;
		if(new_mask != mask)
			SetCommMask(h, mask = new_mask);
		WaitCommEvent(h, &mask, 0);
		break;
	}

	return 0;
}



static HANDLE std_h[2] = { (HANDLE)((char*)0 + 3), (HANDLE)((char*)0 + 7) };


void _get_console()
{
	AllocConsole();
}

void _hide_console()
{
	FreeConsole();
}


int tcgetattr(int fd, struct termios* termios_p)
{
	if(fd >= 2)
		return -1;
	HANDLE h = std_h[fd];

	DWORD mode;
	GetConsoleMode(h, &mode);
	termios_p->c_lflag = mode & (ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT);

	return 0;
}


int tcsetattr(int fd, int /* optional_actions */, const struct termios* termios_p)
{
	if(fd >= 2)
		return -1;
	HANDLE h = std_h[fd];
	SetConsoleMode(h, (DWORD)termios_p->c_lflag);
	FlushConsoleInputBuffer(h);

	return 0;
}


int poll(struct pollfd /* fds */[], int /* nfds */, int /* timeout */)
{
	return -1;
}

