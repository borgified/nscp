/**************************************************************************
*   Copyright (C) 2004-2007 by Michael Medin <michael@medin.name>         *
*                                                                         *
*   This code is part of NSClient++ - http://trac.nakednuns.org/nscp      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
//#include "StdAfx.h"
#include <windows.h>


#include <sysinfo.h>
#include <tchar.h>
#include <error.hpp>

namespace systemInfo {
		LANGID GetSystemDefaultUILanguage() {
				HMODULE hKernel = ::LoadLibrary(_TEXT("KERNEL32"));
				if (!hKernel) 
					throw SystemInfoException("Could not load kernel32.dll: " + error::lookup::last_error());
				tGetSystemDefaultUILanguage fGetSystemDefaultUILanguage;
				fGetSystemDefaultUILanguage = (tGetSystemDefaultUILanguage)::GetProcAddress(hKernel, "GetSystemDefaultUILanguage");
				if (!fGetSystemDefaultUILanguage)
						throw SystemInfoException("Could not load GetSystemDefaultUILanguage" + error::lookup::last_error());
				return fGetSystemDefaultUILanguage();
			}
	}

