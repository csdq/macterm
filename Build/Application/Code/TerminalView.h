/*!	\file TerminalView.h
	\brief Renders the contents of a terminal screen buffer and
	handles interaction with the user (such as selection of
	text).
	
	This is the Terminal View module, which defines the visual
	component of a terminal screen and tends to refer to screen
	coordinates in pixels.  Compare this to the Terminal Screen
	module (Terminal.h), which works with terminal screens in
	terms of the data in them, and tends to refer to screen
	coordinates in rows and columns.
	
	Generally, you only use Terminal View APIs to manipulate
	things that are unique to user interaction with a terminal,
	such as the text selection.  Anything that is data-centric
	should be manipulated from the Terminal Screen standpoint
	(see Terminal.h), because data changes will eventually be
	propagated to the view for rendering.  So, expect only the
	Terminal Screen module to use most of these APIs.
*/
/*###############################################################

	MacTerm
		© 1998-2018 by Kevin Grant.
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

#include <UniversalDefines.h>

#pragma once

// standard-C++ includes
#include <utility>
#include <vector>

// Mac includes
#ifdef __OBJC__
#include <AppKit/AppKit.h>
#else
class NSView;
class NSWindow;
#endif
#include <Carbon/Carbon.h>

// library includes
#ifdef __OBJC__
#	include <CoreUI.objc++.h>
#endif
#include <ListenerModel.h>

// application includes
#include "Preferences.h"
#include "TerminalRangeDescription.typedef.h"
#include "TerminalScreenRef.typedef.h"



#pragma mark Constants

/*!
Possible return values from certain APIs in this module.
*/
enum TerminalView_Result
{
	kTerminalView_ResultOK = 0,					//!< no error
	kTerminalView_ResultInvalidID = -1,			//!< a given "TerminalViewRef" does not correspond to any known view
	kTerminalView_ResultParameterError = -2,	//!< invalid input (e.g. a null pointer)
	kTerminalView_ResultNotEnoughMemory = -3,	//!< there is not enough memory to allocate required data structures
	kTerminalView_ResultIllegalOperation = -4   //!< attempt to change a setting that is currently automatically-controlled
};

/*!
Identifiers for the “custom” colors of a terminal view.
*/
typedef SInt16 TerminalView_ColorIndex;
enum
{
	kTerminalView_ColorIndexNormalText				= 0,
	kTerminalView_ColorIndexNormalBackground		= 1,
	kTerminalView_ColorIndexBlinkingText			= 2,
	kTerminalView_ColorIndexBlinkingBackground		= 3,
	kTerminalView_ColorIndexBoldText				= 4,
	kTerminalView_ColorIndexBoldBackground			= 5,
	kTerminalView_ColorIndexCursorBackground		= 6,
	kTerminalView_ColorIndexMatteBackground			= 7,
	
	// useful constants
	kTerminalView_ColorIndexFirstValid				= kTerminalView_ColorIndexNormalText,
	kTerminalView_ColorIndexLastValid				= kTerminalView_ColorIndexMatteBackground
};

/*!
Determines how the pixel area of the container control is filled
with terminal content.  Normally, the font is a specific size, and
the content either fits or doesn’t fit the pixel area.  However, in
zoom mode, the content is forced to fit in the pixel area; the font
size changes to whatever size makes the content best fit the area.
*/
enum TerminalView_DisplayMode
{
	kTerminalView_DisplayModeNormal		= 'Norm',   //!< underlying terminal screen’s dimensions are altered to
													//!  best suit the pixel dimensions of the view, when resized;
													//!  the terminal view font size is unchanged
	kTerminalView_DisplayModeZoom		= 'Zoom'	//!< font size of text in view is altered to make the current
													//!  rows and columns best fill the screen area, when resized;
													//!  the underlying terminal screen’s dimensions are unchanged
};

/*!
Events in a Terminal View that other modules can register to
receive notification of.
*/
enum TerminalView_Event
{
	kTerminalView_EventFontSizeChanged			= 'FSiz',	//!< the font size used for drawing text has been altered
															//!  (context: TerminalViewRef)
	kTerminalView_EventScrolling				= 'Scrl',	//!< the visible part of the terminal view has changed
															//!  (context: TerminalViewRef)
	kTerminalView_EventSearchResultsExistence	= 'Srch'	//!< the result of TerminalView_SearchResultsExist() is now
															//!  different; this is NOT called if the number of search
															//!  results simply changes (context: TerminalViewRef)
};

/*!
Options for TerminalView_ReturnSelectedTextCopyAsUnicode().
*/
typedef UInt16 TerminalView_TextFlags;
enum
{
	kTerminalView_TextFlagInline					= (1 << 0),		//!< strip end-of-line markers?
	kTerminalView_TextFlagLineSeparatorLF 		= (1 << 1),		//!< use LF as line ending (default is CR)
	kTerminalView_TextFlagLastLineHasSeparator 	= (1 << 2)		//!< also add end-of-line to end of text? (default is no)
};

#pragma mark Types

#include "TerminalViewRef.typedef.h"

/*!
This object wraps pixel values to guard against accidental
conversions or other misuse (such as a value in units other
than pixels).  It also stores both the precise and pixel-grid
version of a pixel value, allowing Core Graphics renderings
to retain exact calculation results that cannot be preserved
in legacy QuickDraw views.  QuickDraw support will eventually
be removed.

The storage sizes are also template parameters so that this
can use less space if the pixel range is not expected to be
big (for example, terminal display width versus the entire
pixel range of the terminal scrollback region).
*/
template < typename discrete_type, typename precise_type >
struct TerminalView_PixelValue
{
public:
    explicit TerminalView_PixelValue ()
    : pixels_(0.0)
    {
    }
	
    void
	setIntegralPixels	(discrete_type		inIntegralPixelCount)
    {
		pixels_ = inIntegralPixelCount;
    }
	
    void
	setPrecisePixels	(precise_type		inExactPixelRange)
    {
		pixels_ = inExactPixelRange;
    }
	
	discrete_type
	integralPixels () const
	{
		return STATIC_CAST(pixels_, discrete_type);
	}
	
	precise_type
	precisePixels () const
	{
		return pixels_;
	}

private:
    precise_type	pixels_; // Core Graphics high-precision value
};

typedef TerminalView_PixelValue< SInt16, Float32 >	TerminalView_PixelWidth;
typedef TerminalView_PixelValue< SInt32, CGFloat >	TerminalView_PixelHeight;

/*!
Since a terminal view can have a potentially huge scrollback
buffer, it is important to use the data type below (and not
just some integer) to represent an index for a row.  Note
that a row index is signed because negative values indicate
scrollback rows and positive values indicate screen rows.
*/
typedef SInt64												TerminalView_RowIndex;

typedef std::pair< UInt16, TerminalView_RowIndex >			TerminalView_Cell;		//!< order: column, row

typedef std::pair< TerminalView_Cell, TerminalView_Cell >	TerminalView_CellRange;	//!< order: inclusive start, exclusive end

typedef std::vector< TerminalView_CellRange >				TerminalView_CellRangeList;

#ifdef __OBJC__

@class TerminalView_Controller;


/*!
Block used for iteration over terminal view controllers.
*/
typedef void (^TerminalView_ControllerBlock)(TerminalView_Controller*, BOOL*);


/*!
Used to notify another view when a click occurs in a background
view (e.g. when a click in the matte region should be mapped
to a click in the nearby terminal itself).

Any view that implements this should probably also implement
mouse-over cursors to be consistent with its behavior.
*/
@protocol TerminalView_ClickDelegate < NSObject > //{

@required

	// notification about a mouse-down event in the specified view
	- (void)
	didReceiveMouseDownEvent:(NSEvent*)_
	forView:(NSView*)_;

@optional

	// notification about a mouse-dragged event in the specified view
	- (void)
	didReceiveMouseDraggedEvent:(NSEvent*)_
	forView:(NSView*)_;

	// notification about a mouse-up event in the specified view
	- (void)
	didReceiveMouseUpEvent:(NSEvent*)_
	forView:(NSView*)_;

@end //}


/*!
Implements the background rendering part of the
Terminal View.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_BackgroundView : NSControl //{
{
@private
	id< TerminalView_ClickDelegate >	_clickDelegate;
	size_t								_colorIndex;
	NSColor*							_exactColor;
	void*								_internalViewPtr;
}

// accessors
	@property (assign) id< TerminalView_ClickDelegate >
	clickDelegate;
	@property (strong) NSColor*
	exactColor;

@end //}


/*!
Declares a series of text-input methods that make sense
for interacting with a terminal view.  No other ordinary
text input methods are expected (namely, this is not a
sub-protocol of NSTextInputClient).
*/
@protocol TerminalView_TextInputClient //{

@required

	// process the given string as user input (send to a session)
	- (void)
	receivedString:(NSString*)_
	terminalView:(TerminalViewRef)_;

	// user input delete key (send appropriate sequence to a session)
	- (void)
	receivedDeleteBackwardInTerminalView:(TerminalViewRef)_;

	// user input newline (send appropriate sequence to a session)
	- (void)
	receivedNewlineInTerminalView:(TerminalViewRef)_;

@end //}


/*!
Implements the main rendering part of the Terminal View.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_ContentView : NSControl < NSTextInputClient > //{
{
@private
	BOOL								_showDragHighlight;
	NSUInteger							_modifierFlagsForCursor;
	void*								_internalViewPtr;
	id< TerminalView_TextInputClient >	_textInputDelegate;
}

// new methods
	- (TerminalViewRef)
	terminalViewRef;

// accessors
	@property (assign) id< TerminalView_TextInputClient >
	textInputDelegate;

// actions
	- (IBAction)
	performFormatByFavoriteName:(id)_;
	- (IBAction)
	performFormatDefault:(id)_;
	- (IBAction)
	performKill:(id)_;
	- (IBAction)
	performRestart:(id)_;
	- (IBAction)
	performSaveSelection:(id)_;
	- (IBAction)
	performTranslationSwitchByFavoriteName:(id)_;
	- (IBAction)
	performTranslationSwitchDefault:(id)_;
	- (IBAction)
	startSpeaking:(id)_;
	- (IBAction)
	stopSpeaking:(id)_;

@end //}


/*!
The type of view managed by "TerminalView_Controller".
Currently used to handle layout (until a later SDK can
be adopted, where the view controller might directly
perform more view tasks).
*/
@interface TerminalView_Object : CoreUI_LayoutView //{
{
@private
	void*								_internalViewPtr;
	TerminalView_ContentView*			_terminalContentView;
	TerminalView_BackgroundView*		_terminalMarginViewBottom;
	TerminalView_BackgroundView*		_terminalMarginViewLeft;
	TerminalView_BackgroundView*		_terminalMarginViewRight;
	TerminalView_BackgroundView*		_terminalMarginViewTop;
	TerminalView_BackgroundView*		_terminalPaddingViewBottom;
	TerminalView_BackgroundView*		_terminalPaddingViewLeft;
	TerminalView_BackgroundView*		_terminalPaddingViewRight;
	TerminalView_BackgroundView*		_terminalPaddingViewTop;
}

// initializers
	- (instancetype)
	initWithCoder:(NSCoder*)_;
	- (instancetype)
	initWithFrame:(NSRect)_;

// accessors
	@property (strong) TerminalView_ContentView*
	terminalContentView;
	@property (strong) TerminalView_BackgroundView*
	terminalMarginViewBottom;
	@property (strong) TerminalView_BackgroundView*
	terminalMarginViewLeft;
	@property (strong) TerminalView_BackgroundView*
	terminalMarginViewRight;
	@property (strong) TerminalView_BackgroundView*
	terminalMarginViewTop;
	@property (strong) TerminalView_BackgroundView*
	terminalPaddingViewBottom;
	@property (strong) TerminalView_BackgroundView*
	terminalPaddingViewLeft;
	@property (strong) TerminalView_BackgroundView*
	terminalPaddingViewRight;
	@property (strong) TerminalView_BackgroundView*
	terminalPaddingViewTop;

@end //}


/*!
Implements a view controller for the Cocoa version of
the terminal view.  See "TerminalWindowCocoa.xib".

A TerminalViewRef owns this controller, created as a
side effect of TerminalView_NewNSViewBased().

Window elements are handled by TerminalWindow_Controller.

Note that this is only in the header for the sake of
Interface Builder, which will not synchronize with
changes to an interface declared in a ".mm" file.
*/
@interface TerminalView_Controller : NSViewController < CoreUI_ViewLayoutDelegate > //{
{
}

// initializers
	- (instancetype)
	init;

// accessors
	- (TerminalView_Object*)
	terminalView;

@end //}


/*!
Tweaks a standard scroll bar to provide extra features
such as tick marks to show search results.
*/
@interface TerminalView_ScrollBar : NSScroller //{
{
@private
	void*	_internalViewPtr;
}

@end //}


/*!
The type of view managed by "TerminalView_ScrollableRootVC".
Currently used to handle layout (until a later SDK can be
adopted, where the view controller might directly perform
more view tasks).
*/
@interface TerminalView_ScrollableRootView : CoreUI_LayoutView //@{
{
@private
	void*						_internalViewPtr;
	TerminalView_ScrollBar*		_scrollBarV;
}

// initializers
	- (instancetype)
	initWithCoder:(NSCoder*)_;
	- (instancetype)
	initWithFrame:(NSRect)_;

// accessors
	@property (strong) TerminalView_ScrollBar*
	scrollBarV;

@end //}


/*!
Custom root view controller that holds a scroll bar and
zero or more terminal view controllers.  This is also
responsible for arranging a scroll bar next to its view.
*/
@interface TerminalView_ScrollableRootVC : NSViewController < CoreUI_ViewLayoutDelegate > //{
{
@private
	TerminalView_ScrollBar*		_scrollBarV;
	NSMutableArray*				_terminalViewControllers;
}

// initializers
	- (instancetype)
	init;

// accessors
	- (TerminalView_ScrollableRootView*)
	scrollableRootView;

// new methods
	- (void)
	addTerminalViewController:(TerminalView_Controller*)_;
	- (void)
	enumerateTerminalViewControllersUsingBlock:(TerminalView_ControllerBlock)_;
	- (void)
	removeTerminalViewController:(TerminalView_Controller*)_;

@end //}

#else

class TerminalView_Object;

#endif // __OBJC__



#pragma mark Public Methods

//!\name Initialization
//@{

void
	TerminalView_Init				();

void
	TerminalView_Done				();

//@}

//!\name Creating and Destroying Terminal Views
//@{

TerminalViewRef
	TerminalView_NewNSViewBased		(TerminalView_Object*			inRootView,
									 TerminalScreenRef				inScreenDataSource,
									 Preferences_ContextRef			inFormatOrNull = nullptr);

//@}

//!\name Modifying Terminal View Data
//@{

TerminalView_Result
	TerminalView_AddDataSource				(TerminalViewRef			inView,
											 TerminalScreenRef			inScreenDataSource);

TerminalView_Result
	TerminalView_RemoveDataSource			(TerminalViewRef			inView,
											 TerminalScreenRef			inScreenDataSourceOrNull);

//@}

//!\name Event Notification
//@{

TerminalView_Result
	TerminalView_IgnoreChangesToPreference	(TerminalViewRef			inView,
											 Preferences_Tag			inWhichSetting);

TerminalView_Result
	TerminalView_StartMonitoring			(TerminalViewRef			inRef,
											 TerminalView_Event			inForWhatEvent,
											 ListenerModel_ListenerRef	inListener);

TerminalView_Result
	TerminalView_StopMonitoring				(TerminalViewRef			inRef,
											 TerminalView_Event			inForWhatEvent,
											 ListenerModel_ListenerRef	inListener);

//@}

//!\name Hit Testing
//@{

Boolean
	TerminalView_PtInSelection		(TerminalViewRef	inView,
									 Point				inLocalPoint);

//@}

//!\name Managing the Text Selection
//@{

void
	TerminalView_DisplayCompletionsUI			(TerminalViewRef				inView);

void
	TerminalView_DisplaySaveSelectionUI			(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindNothing					(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_FindVirtualRange				(TerminalViewRef				inView,
												 TerminalView_CellRange const&	inSelection);

void
	TerminalView_FlashSelection					(TerminalViewRef				inView);

TerminalView_Result
	TerminalView_GetSearchResults				(TerminalViewRef				inView,
												 TerminalView_CellRangeList&	outResults);

void
	TerminalView_GetSelectedTextAsAudio			(TerminalViewRef				inView);

void
	TerminalView_GetSelectedTextAsVirtualRange	(TerminalViewRef				inView,
												 TerminalView_CellRange&		outSelection);

void
	TerminalView_MakeSelectionsRectangular		(TerminalViewRef				inView,
												 Boolean						inAreSelectionsNotAttachedToScreenEdges);

CFStringRef
	TerminalView_ReturnCursorWordCopyAsUnicode	(TerminalViewRef				inView);

CFArrayRef
	TerminalView_ReturnSelectedImageArrayCopy	(TerminalViewRef				inView);

// INEFFICIENT, USE WITH CARE; LOOK FOR OTHER APIS THAT CAN READ THE SELECTION WITHOUT COPYING IT
CFStringRef
	TerminalView_ReturnSelectedTextCopyAsUnicode	(TerminalViewRef				inView,
													 UInt16							inNumberOfSpacesToReplaceWithOneTabOrZero,
													 TerminalView_TextFlags			inFlags);

size_t
	TerminalView_ReturnSelectedTextSize			(TerminalViewRef				inView);

Boolean
	TerminalView_SearchResultsExist				(TerminalViewRef				inView);

void
	TerminalView_SelectBeforeCursorCharacter	(TerminalViewRef				inView);

void
	TerminalView_SelectCursorCharacter			(TerminalViewRef				inView);

void
	TerminalView_SelectCursorLine				(TerminalViewRef				inView);

void
	TerminalView_SelectEntireBuffer				(TerminalViewRef				inView);

void
	TerminalView_SelectMainScreen				(TerminalViewRef				inView);

void
	TerminalView_SelectNothing					(TerminalViewRef				inView);

void
	TerminalView_SelectVirtualRange				(TerminalViewRef				inView,
												 TerminalView_CellRange const&	inSelection);

TerminalView_Result
	TerminalView_SetTextSelectionRenderingEnabled	(TerminalViewRef			inView,
												 Boolean						inIsSelectionEnabled);

Boolean
	TerminalView_TextSelectionExists			(TerminalViewRef				inView);

Boolean
	TerminalView_TextSelectionIsRectangular		(TerminalViewRef				inView);

//@}

//!\name Window Management
//@{

NSWindow*
	TerminalView_ReturnNSWindow					(TerminalViewRef			inView);

//@}

//!\name Visible Area
//@{

TerminalView_Result
	TerminalView_GetScrollVerticalInfo			(TerminalViewRef			inView,
												 SInt64&					outStartOfRange,
												 SInt64&					outPastEndOfRange,
												 SInt64&					outStartOfMaximum,
												 SInt64&					outPastEndOfMaximum);

TerminalView_DisplayMode
	TerminalView_ReturnDisplayMode				(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollAround					(TerminalViewRef			inView,
												 SInt16						inColumnCountDelta,
												 SInt16						inRowCountDelta);

TerminalView_Result
	TerminalView_ScrollColumnsTowardLeftEdge	(TerminalViewRef			inView,
												 UInt16						inNumberOfColumnsToScroll);

TerminalView_Result
	TerminalView_ScrollColumnsTowardRightEdge	(TerminalViewRef			inView,
												 UInt16						inNumberOfColumnsToScroll);

TerminalView_Result
	TerminalView_ScrollPageTowardBottomEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardLeftEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardRightEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollPageTowardTopEdge		(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollRowsTowardBottomEdge		(TerminalViewRef			inView,
												 UInt32						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollRowsTowardTopEdge		(TerminalViewRef			inView,
												 UInt32						inNumberOfRowsToScroll);

TerminalView_Result
	TerminalView_ScrollToBeginning				(TerminalViewRef			inView);

TerminalView_Result
	TerminalView_ScrollToCell					(TerminalViewRef				inView,
												 TerminalView_Cell const&	inCell);

TerminalView_Result
	TerminalView_ScrollToEnd					(TerminalViewRef			inView);

// USE TerminalView_GetScrollVerticalInfo() TO DETERMINE APPROPRIATE VALUES FOR THESE INTEGER RANGES
TerminalView_Result
	TerminalView_ScrollToIndicatorPosition		(TerminalViewRef			inView,
												 SInt32						inStartOfVerticalRange,
												 SInt32						inStartOfHorizontalRange = 0);

TerminalView_Result
	TerminalView_SetDisplayMode					(TerminalViewRef			inView,
												 TerminalView_DisplayMode   inNewMode);

TerminalView_Result
	TerminalView_SetFocusRingDisplayed			(TerminalViewRef			inView,
												 Boolean					inShowFocusRingAndMatte);

//@}

//!\name Cursor Management
//@{

TerminalView_Result
	TerminalView_GetCursorGlobalBounds			(TerminalViewRef			inView,
												 CGRect&					outGlobalBounds);

void
	TerminalView_MoveCursorWithArrowKeys		(TerminalViewRef			inView,
												 Point						inLocalMouse);

//@}

//!\name Metrics
//@{

Boolean
	TerminalView_GetIdealSize					(TerminalViewRef			inView,
												 TerminalView_PixelWidth&	outWidthInPixels,
												 TerminalView_PixelHeight&	outHeightInPixels);

void
	TerminalView_GetTheoreticalScreenDimensions	(TerminalViewRef			inView,
												 TerminalView_PixelWidth	inWidthInPixels,
												 TerminalView_PixelHeight	inHeightInPixels,
												 UInt16*					outColumnCount,
												 TerminalView_RowIndex*		outRowCount);

void
	TerminalView_GetTheoreticalViewSize			(TerminalViewRef			inView,
												 UInt16						inColumnCount,
												 TerminalView_RowIndex		inRowCount,
												 TerminalView_PixelWidth&	outWidthInPixels,
												 TerminalView_PixelHeight&	outHeightInPixels);

//@}

//!\name Cocoa NSView Management
//@{

void
	TerminalView_FocusForUser					(TerminalViewRef			inView);

TerminalView_Object*
	TerminalView_ReturnContainerNSView			(TerminalViewRef			inView);

NSView*
	TerminalView_ReturnDragFocusNSView			(TerminalViewRef			inView);

NSView*
	TerminalView_ReturnUserFocusNSView			(TerminalViewRef			inView);

//@}

//!\name Appearance
//@{

Boolean
	TerminalView_GetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 CGFloatRGBColor*			outColorPtr);

void
	TerminalView_GetFontAndSize					(TerminalViewRef			inView,
												 CFStringRef*				outFontFamilyNameOrNull,
												 UInt16*					outFontSizeOrNull);

Preferences_ContextRef
	TerminalView_ReturnFormatConfiguration		(TerminalViewRef			inView);

Preferences_ContextRef
	TerminalView_ReturnTranslationConfiguration	(TerminalViewRef			inView);

void
	TerminalView_ReverseVideo					(TerminalViewRef			inView,
												 Boolean					inReverseVideo);

Boolean
	TerminalView_SetColor						(TerminalViewRef			inView,
												 TerminalView_ColorIndex	inColorEntryNumber,
												 CGFloatRGBColor const*		inColorPtr);

TerminalView_Result
	TerminalView_SetFontAndSize					(TerminalViewRef			inView,
												 CFStringRef				inFontFamilyNameOrNull,
												 Float32					inFontSizeOrZero);

//@}

//!\name State Management
//@{

TerminalView_Result
	TerminalView_SetCursorRenderingEnabled		(TerminalViewRef			inView,
												 Boolean					inIsCursorVisible);

void
	TerminalView_SetDrawingEnabled				(TerminalViewRef			inView,
												 Boolean					inIsDrawingEnabled);

TerminalView_Result
	TerminalView_SetResizeScreenBufferWithView	(TerminalViewRef			inView,
												 Boolean					inScreenDimensionsAutoSync);

TerminalView_Result
	TerminalView_SetUserInteractionEnabled		(TerminalViewRef			inView,
												 Boolean					inIsInteractionEnabled);

//@}

//!\name Miscellaneous
//@{

void
	TerminalView_DeleteScrollback				(TerminalViewRef					inView);

void
	TerminalView_RotateSearchResultHighlight	(TerminalViewRef					inView,
												 SInt16								inHowFarWhichWay);

TerminalView_Result
	TerminalView_TranslateTerminalScreenRange	(TerminalViewRef					inView,
												 Terminal_RangeDescription const&	inRange,
												 TerminalView_CellRange&			outRange);

void
	TerminalView_ZoomOpenFromSelection			(TerminalViewRef					inView);

void
	TerminalView_ZoomToCursor					(TerminalViewRef					inView);

void
	TerminalView_ZoomToSearchResults			(TerminalViewRef					inView);

//@}

// BELOW IS REQUIRED NEWLINE TO END FILE
