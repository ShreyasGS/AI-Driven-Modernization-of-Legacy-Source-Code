#import "MyBrowserView.h"

#define DOCUMENT_DONE_STRING @"Document: Done"
#define LOADING_STRING @"Loading..."

@implementation MyBrowserView

- (IBAction)load:(id)sender
{
  NSString* str = [urlbar stringValue];
  NSURL* url = [NSURL URLWithString:str];
  [browserView loadURI:url flags:NSLoadFlagsNone];
}

- (void)awakeFromNib 
{
  NSRect bounds = [self bounds];
  browserView = [[NSBrowserView alloc] initWithFrame:bounds];
  [self addSubview:browserView];
  [browserView setContainer:self];
  [browserView addListener:self];

  defaultStatus = NULL;
  loadingStatus = DOCUMENT_DONE_STRING;
  [status setStringValue:loadingStatus];

  [progress retain];
  [progress removeFromSuperview];
}

- (void)setFrame:(NSRect)frameRect
{
  [super setFrame:frameRect];
  NSRect bounds = [self bounds];
  [browserView setFrame:bounds];
}

- (void)onLoadingStarted 
{
  if (defaultStatus) {
    [defaultStatus release];
    defaultStatus = NULL;
  }

  [progressSuper addSubview:progress];
  [progress release];
  [progress setIndeterminate:YES];
  [progress startAnimation:self];

  loadingStatus = LOADING_STRING;
  [status setStringValue:loadingStatus];

#ifdef DEBUG_vidur
  printf("Starting to load\n");
#endif
}

- (void)onLoadingCompleted:(BOOL)succeeded
{
  [progress setIndeterminate:YES];
  [progress stopAnimation:self];
  [progress retain];
  [progress removeFromSuperview];

  loadingStatus = DOCUMENT_DONE_STRING;
  if (defaultStatus) {
    [status setStringValue:defaultStatus];
  }
  else {
    [status setStringValue:loadingStatus];
  }

#ifdef DEBUG_vidur
  printf("Loading completed\n");
#endif
}

- (void)onProgressChange:(int)currentBytes outOf:(int)maxBytes 
{
  if (maxBytes > 0) {
    BOOL isIndeterminate = [progress isIndeterminate];
    if (isIndeterminate) {
      [progress setIndeterminate:NO];
    }
    double val = ((double)currentBytes / (double)maxBytes) * 100.0;
    [progress setDoubleValue:val];
#ifdef DEBUG_vidur
    printf("Progress notification: %f%%\n", val);
#endif
  }
}

- (void)onLocationChange:(NSURL*)url 
{
  NSString* spec = [url absoluteString];
  [urlbar setStringValue:spec];
  
#ifdef DEBUG_vidur
  const char* str = [spec cString];
  printf("Location changed to: %s\n", str);
#endif
}

- (void)setStatus:(NSString *)statusString ofType:(NSStatusType)type 
{
  if (type == NSStatusTypeScriptDefault) {
    if (defaultStatus) {
      [defaultStatus release];
    }
    defaultStatus = statusString;
    if (defaultStatus) {
      [defaultStatus retain];
    }
  }
  else if (!statusString) {
    if (defaultStatus) {
      [status setStringValue:defaultStatus];
    }
    else {
      [status setStringValue:loadingStatus];
    }      
  }
  else {
    [status setStringValue:statusString];
  }
}

- (NSString *)title 
{
  NSWindow* window = [self window];
  NSString* str = [window title];
  return str;
}

- (void)setTitle:(NSString *)title
{
  NSWindow* window = [self window];
  [window setTitle:title];
}

- (void)sizeBrowserTo:(NSSize)dimensions
{
  NSRect bounds = [self bounds];
  float dx = dimensions.width - bounds.size.width;
  float dy = dimensions.height - bounds.size.height;

  NSWindow* window = [self window];
  NSRect frame = [window frame];
  frame.size.width += dx;
  frame.size.height += dy;

  [window setFrame:frame display:YES];
}

- (NSBrowserView*)createBrowserWindow:(unsigned int)mask
{
  // XXX not implemented 
  return NULL;
}

@end
