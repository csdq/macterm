/*!	\file Popover.objc++.h
	\brief Implements a popover-style window.
	
	Unlike popovers in Lion, these function on many older
	versions of Mac OS X (tested back to Mac OS X 10.3).
	They are also more flexible, allowing different colors
	for instance.
*/
/*###############################################################

	Popover Window 1.1 (based on MAAttachedWindow)
	MAAttachedWindow © 2007 by Magic Aubergine
	Popover Window © 2011-2012 by Kevin Grant
	
	Based on the MAAttachedWindow code created by Matt Gemmell
	on September 27, 2007 and distributed under the terms in
	the file "Licenses/MattGemmell.txt" located in the MacTerm
	source distribution.
	
	Popover Window adds Mac OS X 10.3 compatibility, makes the
	window more Cocoa-compatible (animated resizing now works,
	for example) and adds a better auto-positioning algorithm,
	among other enhancements.  Code has also been reworked to
	match MacTerm standards.

###############################################################*/

// Mac includes
#import <Cocoa/Cocoa.h>


#pragma mark Constants

/*!
Window properties of the same type occupy the same bit range, but
unrelated properties are in different ranges; they can therefore be
combined.  For example, a window can be positioned to the left of
an arrow that is in the center position.

The "kPopover_PropertyArrow..." constants determine where the arrow
appears along its edge.  The “beginning” is relative to the top-left
corner of the window so an arrow pointing vertically on the rightmost
part of the top or bottom window edge would be at the “end” of its
edge (as would an arrow pointing horizontally on the bottommost part
of the left or right window edges).

The "kPopover_PlaceFrame..." constants determine where the window
frame appears to be from the user’s point of view relative to the
arrow (in reality the window occupies a bigger frame).
*/
typedef unsigned int Popover_Properties;
enum
{
	kPopover_PropertyShiftArrow					= 0, // shift only (invalid value)
	kPopover_PropertyMaskArrow					= (0x03 << kPopover_PropertyShiftArrow), // mask only (invalid value)
	kPopover_PropertyArrowMiddle				= (0x00 << kPopover_PropertyShiftArrow),
	kPopover_PropertyArrowBeginning				= (0x01 << kPopover_PropertyShiftArrow),
	kPopover_PropertyArrowEnd					= (0x02 << kPopover_PropertyShiftArrow),
	
	kPopover_PropertyShiftPlaceFrame			= 2, // shift only (invalid value)
	kPopover_PropertyMaskPlaceFrame				= (0x03 << kPopover_PropertyShiftPlaceFrame), // mask only (invalid value)
	kPopover_PropertyPlaceFrameBelowArrow		= (0x00 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameLeftOfArrow		= (0x01 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameRightOfArrow		= (0x02 << kPopover_PropertyShiftPlaceFrame),
	kPopover_PropertyPlaceFrameAboveArrow		= (0x03 << kPopover_PropertyShiftPlaceFrame)
};


/*!
DEPRECATED.  Use window properties individually.

Popover window positions are relative to the point passed to the
constructor, e.g. kPopover_PositionBottomRight will put the window
below the point and towards the right, kPopover_PositionTop will
horizontally center it above the point, kPopover_PositionRightTop
will put the window to the right and above the point, and so on.

Note that it is also possible to request automatic positioning using
"setPointWithAutomaticPositioning:preferredSide:".  If that is used
then the window is given the “best” possible position but the given
preferred side is used if that side is tied with any other candidate.
*/
enum
{
	kPopover_PositionBottom			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionBottomRight	= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionBottomLeft		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameBelowArrow,
	kPopover_PositionLeft			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionLeftBottom		= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionLeftTop		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameLeftOfArrow,
	kPopover_PositionRight			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionRightBottom	= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionRightTop		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameRightOfArrow,
	kPopover_PositionTop			= kPopover_PropertyArrowMiddle | kPopover_PropertyPlaceFrameAboveArrow,
	kPopover_PositionTopRight		= kPopover_PropertyArrowBeginning | kPopover_PropertyPlaceFrameAboveArrow,
	kPopover_PositionTopLeft		= kPopover_PropertyArrowEnd | kPopover_PropertyPlaceFrameAboveArrow
};


#pragma mark Types

/*!
A popover-style window that works on many versions of Mac OS X.

This class handles only the visual parts of a popover, not the
equally-important behavioral aspects.  To help display and manage
this window, see "PopoverManager.objc++.h".

Note that accessor methods are generally meant to configure the
window before displaying it.  The user should not normally see
the window change its appearance while it is on screen.  One
exception to this is "setHasArrow:", which will simply update the
frame appearance.
*/
@interface Popover_Window : NSWindow
{
@private
	__weak NSWindow*		popoverParentWindow;
	__weak NSView*			embeddedView;
	NSColor*				borderOuterColor;
	NSColor*				borderPrimaryColor;
	NSColor*				popoverBackgroundColor;
	NSRect					viewFrame;
	float					arrowBaseWidth;
	float					arrowHeight;
	float					borderWidth;
	float					cornerRadius;
	float					viewMargin;
	BOOL					resizeInProgress;
	BOOL					hasArrow;
	BOOL					hasRoundCornerBesideArrow;
	BOOL					isAutoPositioning;
	Popover_Properties		windowPropertyFlags;
}

// designated initializer
- (id)
initWithView:(NSView*)_
attachedToPoint:(NSPoint)_
inWindow:(NSWindow*)_;

// window location

- (void)
setPoint:(NSPoint)_
onSide:(Popover_Properties)_;

- (void)
setPointWithAutomaticPositioning:(NSPoint)_
preferredSide:(Popover_Properties)_;

// utilities

- (NSRect)
viewRectForFrameRect:(NSRect)_;

- (NSRect)
frameRectForViewRect:(NSRect)_;

// accessors

- (float)
arrowBaseWidth;
- (void)
setArrowBaseWidth:(float)_;

- (float)
arrowHeight;
- (void)
setArrowHeight:(float)_;

- (NSColor*)
borderOuterColor;
- (void)
setBorderOuterColor:(NSColor*)_;

- (NSColor*)
borderPrimaryColor;
- (void)
setBorderPrimaryColor:(NSColor*)_;

- (float)
borderWidth;
- (void)
setBorderWidth:(float)_;

- (float)
cornerRadius;
- (void)
setCornerRadius:(float)_;

- (BOOL)
hasRoundCornerBesideArrow;
- (void)
setHasRoundCornerBesideArrow:(BOOL)_;

- (BOOL)
hasArrow;
- (void)
setHasArrow:(BOOL)_;

- (NSColor*)
popoverBackgroundColor;
- (void)
setPopoverBackgroundColor:(NSColor*)_;

- (float)
viewMargin;
- (void)
setViewMargin:(float)_;

// NSWindow

- (void)
setBackgroundColor:(NSColor*)_; // DO NOT USE; RESERVED FOR RENDERING

@end

// BELOW IS REQUIRED NEWLINE TO END FILE