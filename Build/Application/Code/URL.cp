/*###############################################################

	URL.cp
	
	MacTelnet
		© 1998-2009 by Kevin Grant.
		© 2001-2003 by Ian Anderson.
		© 1986-1994 University of Illinois Board of Trustees
		(see About box for full list of U of I contributors).
	
	This program is free software; you can redistribute it or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version
	2 of the License, or (at your option) any later version.
	
	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more
	details.
	
	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:
	
		Free Software Foundation, Inc.
		59 Temple Place, Suite 330
		Boston, MA  02111-1307
		USA

###############################################################*/

#include "UniversalDefines.h"

// standard-C includes
#include <cctype>
#include <cstring>

// standard-C++ includes
#include <algorithm>

// Mac includes
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>

// library includes
#include <AlertMessages.h>
#include <CFRetainRelease.h>
#include <Console.h>
#include <Localization.h>
#include <MemoryBlocks.h>
#include <RegionUtilities.h>
#include <SoundSystem.h>

// MacTelnet includes
#include "AppleEventUtilities.h"
#include "AppResources.h"
#include "BasicTypesAE.h"
#include "DialogUtilities.h"
#include "QuillsSession.h"
#include "RecordAE.h"
#include "TerminalScreenRef.typedef.h"
#include "TerminalView.h"
#include "Terminology.h"
#include "URL.h"



#pragma mark Constants

#define CR			0x0D	/* the carriage return character */

#define IS_WHITE_SPACE_CHARACTER(a)			(' ' == (a) || '\t' == (a))
#define IS_WHITE_SPACE_OR_CR_CHARACTER(a)	(IS_WHITE_SPACE_CHARACTER(a) || CR == (a))

#pragma mark Variables

namespace // an unnamed namespace is the preferred replacement for "static" declarations in C++
{
	char*	gURLSchemeNames[] =
	{
		// WARNING:	This MUST match the order of the types
		//			in "InternetPrefs.h", this is assumed
		//			within code in this file.
		":",
		"mailto:",	
		"news:",
		"nntp:",
		"ftp:",
		"http:",
		"gopher:",
		"wais:",
		"telnet:",
		"rlogin:",
		"tn3270:",
		"finger:",
		"whois:",
		"rtsp:",
		"ssh:",
		"sftp:",
		"x-man-page:",
		"file:",
		nullptr // this list must end with a nullptr
	};
}


#pragma mark Public Methods

/*!
Examines the currently-selected text of the specified
terminal view for a valid URL.  If it finds one, the
text is flashed briefly and then the proper Apple Events
are fired to open the URL.

(2.6)
*/
void
URL_HandleForScreenView		(TerminalScreenRef	UNUSED_ARGUMENT(inScreen),
							 TerminalViewRef	inView)
{
	CFRetainRelease		urlObject(TerminalView_ReturnSelectedTextAsNewUnicode
									(inView, 0, kTerminalView_TextFlagInline));
	Boolean				openFailed = true;
	
	
	if (urlObject.exists())
	{
		CFStringRef					urlAsCFString = urlObject.returnCFStringRef();
		UniformResourceLocatorType	urlKind = URL_ReturnTypeFromCFString(urlAsCFString);
		
		
		if (kNotURL != urlKind)
		{
			std::string		urlUTF8;
			
			
			// ideally, this should reuse the URL Apple Event handler;
			// but during the transition to Cocoa, that is more complex
			// than is convenient, so Quills is just called directly
			StringUtilities_CFToUTF8(urlAsCFString, urlUTF8);
			if (false == urlUTF8.empty())
			{
				WindowRef		screenWindow = TerminalView_ReturnWindow(inView);
				RgnHandle		selectionRegion = TerminalView_ReturnSelectedTextAsNewRegion(inView);
				Rect			screenRect;
				Rect			selectionRect;
				
				
				RegionUtilities_GetWindowDeviceGrayRect(screenWindow, &screenRect);
				
				// display “launch application zoom rectangles” if an external helper is used
				if (nullptr != selectionRegion)
				{
					CGrafPtr	oldPort = nullptr;
					GDHandle	oldDevice = nullptr;
					
					
					GetGWorld(&oldPort, &oldDevice);
					SetPortWindowPort(screenWindow);
					LocalToGlobalRegion(selectionRegion);
					SetGWorld(oldPort, oldDevice);
					GetRegionBounds(selectionRegion, &selectionRect);
					TerminalView_FlashSelection(inView);
					(OSStatus)ZoomRects(&selectionRect, &screenRect, 20/* steps, arbitrary */, kZoomAccelerate);
					Memory_DisposeRegion(&selectionRegion);
				}
				
				try
				{
					Quills::Session::handle_url(urlUTF8);
					openFailed = false;
				}
				catch (std::exception const&	e)
				{
					Console_Warning(Console_WriteValueCString, "caught exception while trying to handle URL",
									e.what());
					// in the event of an error, show “dying zoom rectangles”
					SetRect(&selectionRect, 0, 0, 0, 0);
					(OSStatus)ZoomRects(&screenRect, &selectionRect, 20/* steps, arbitrary */, kZoomDecelerate);
				}
			}
		}
	}
	
	if (openFailed)
	{
		Console_Warning(Console_WriteLine, "cannot open URL");
		Sound_StandardAlert();
	}
}// HandleForScreenView


/*!
Uses the appropriate Internet helper application to
handle a URL corresponding to the given constant.
This API is a simple way to cause special addresses
to be opened (for example, the home page for this
program) without knowing the exact URL or the method
required to open it.

Returns "true" only if the open attempt succeeds.

(3.0)
*/
Boolean
URL_OpenInternetLocation	(URL_InternetLocation	inSpecialInternetLocationToOpen)
{
	CFRetainRelease		urlCFString;
	Boolean				result = false;
	
	
	switch (inSpecialInternetLocationToOpen)
	{
	case kURL_InternetLocationApplicationHomePage:
		urlCFString = CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(), CFSTR("MyHomePageURL"));
		if (noErr == URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationApplicationSupportEMail:
		urlCFString = CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(), CFSTR("MySupportEMailURL"));
		if (noErr == URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationApplicationUpdatesPage:
		urlCFString = CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(), CFSTR("MyUpdatesURL"));
		if (noErr == URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationSourceCodeLicense:
		urlCFString = CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(), CFSTR("MySourceCodeLicenseURL"));
		if (noErr == URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	case kURL_InternetLocationSourceForgeProject:
		urlCFString = CFBundleGetValueForInfoDictionaryKey(AppResources_ReturnBundleForInfo(), CFSTR("MySourceForgeProjectURL"));
		if (noErr == URL_ParseCFString(urlCFString.returnCFStringRef()))
		{
			// success!
			result = true;
		}
		break;
	
	default:
		// ???
		break;
	}
	return result;
}// OpenInternetLocation


/*!
Constructs a new session if the specified Core Foundation
string buffer contains a valid URL.  The configuration of
the session tries to match all the information given in
the URL.  If the URL is not something MacTelnet understands,
another application may be launched instead.

WARNING:	You probably want Quills::Session::handle_url()
			instead.  Quills respects script-based callbacks
			first, which is where the bulk of even the
			default implementation (much less any user code)
			exists.  Quills automatically calls this routine
			as a fallback when all else fails.

\retval noErr
if the string is a URL and it is opened successfully

\retval kURLInvalidURLError
if the string is not a URL

\retval (other)
if Launch Services has a problem opening the URL

(3.1)
*/
OSStatus
URL_ParseCFString	(CFStringRef	inURLString)
{
	OSStatus	result = noErr;
	
	
	if (nullptr == inURLString) result = memPCErr;
	else
	{
		UniformResourceLocatorType	urlKind = kNotURL;
		
		
		urlKind = URL_ReturnTypeFromCFString(inURLString);
		if (kNotURL != urlKind)
		{
			CFURLRef	theCFURL = CFURLCreateWithString(kCFAllocatorDefault, inURLString, nullptr/* base URL */);
			
			
			if (nullptr == theCFURL) result = kURLInvalidURLError; // just a guess!
			else
			{
				result = LSOpenCFURLRef(theCFURL, nullptr/* URL of actual item opened */);
				if (noErr != result) Console_WriteValue("URL open error", result);
				CFRelease(theCFURL);
			}
		}
	}
	return result;
}// ParseCFString


/*!
Parses the given Core Foundation string and returns
what kind of URL it seems to represent.

Any whitespace on either end of the string is
stripped prior to checks.

FUTURE: Could probably rely on CFURLGetBytes() and
CFURLGetByteRangeForComponent() to do part of this
gruntwork, only in a way that is flexible to many
more types of URLs.

(3.1)
*/
UniformResourceLocatorType
URL_ReturnTypeFromCFString		(CFStringRef	inURLCFString)
{
	UniformResourceLocatorType	result = kNotURL;
	CFMutableStringRef			urlCopyCFString = CFStringCreateMutableCopy
													(kCFAllocatorDefault, 0/* length or 0 for unlimited */,
														inURLCFString);
	
	
	if (nullptr != urlCopyCFString)
	{
		CFStringTrimWhitespace(urlCopyCFString);
		{
			CFStringEncoding const	kEncoding = kCFStringEncodingUTF8;
			size_t const			kStringLength = CFStringGetLength(urlCopyCFString);
			size_t const			kBufferSize = 1 + CFStringGetMaximumSizeForEncoding
														(kStringLength, kEncoding);
			char*					buffer = nullptr;
			
			
			buffer = new char[kBufferSize];
			if (CFStringGetCString(urlCopyCFString, buffer, kBufferSize, kEncoding))
			{
				result = URL_ReturnTypeFromCharacterRange(buffer, buffer + kStringLength);
			}
			delete [] buffer, buffer = nullptr;
		}
		CFRelease(urlCopyCFString), urlCopyCFString = nullptr;
	}
	
	return result;
}// ReturnTypeFromCFString


/*!
Parses the given data handle, assumed to be a
string, and returns what kind of URL it seems
to represent.

Whitespace is NOT stripped in this version of
the routine.

(3.1)
*/
UniformResourceLocatorType
URL_ReturnTypeFromCharacterRange	(char const*	inBegin,
									 char const*	inPastEnd)
{
	UniformResourceLocatorType	result = kNotURL;
	register SInt16				i = 0;
	
	
	// look for a match on the prefix (e.g. "http:")
	for (i = 0; (nullptr != gURLSchemeNames[i]); ++i)
	{
		char const* const	kCStringPtr = gURLSchemeNames[i];
		
		
		// obviously the URL must be at least as long as its prefix!
		if ((inPastEnd - inBegin) > STATIC_CAST(std::strlen(kCStringPtr), ptrdiff_t))
		{
			if (std::equal(kCStringPtr, kCStringPtr + std::strlen(kCStringPtr), inBegin))
			{
				result = STATIC_CAST(i, UniformResourceLocatorType);
				break;
			}
		}
	}
	return result;
}// ReturnTypeFromCharacterRange


/*!
Searches the text from the given screen surrounding
the given point in the specified view, and if a URL
appears to be beneath the point, the view’s text
selection changes to highlight the URL.

Returns "true" only if a URL was found.

(2.6)
*/
Boolean
URL_SetSelectionToProximalURL	(TerminalView_Cell const&	inCommandClickColumnRow,
								 TerminalScreenRef			inScreen,
								 TerminalViewRef			inView)
{
	// TEMPORARY - REIMPLEMENT
	return false;
}// SetSelectionToProximalURL

// BELOW IS REQUIRED NEWLINE TO END FILE
