#ifndef BCODER_H_

#define internal static
#define global_variable static
#define local_persist static

typedef char s8;
typedef unsigned char u8;

typedef unsigned short u16;
typedef short s16;

typedef unsigned int u32;
typedef int s32;

typedef float r32;
typedef double r64;

#ifndef Assert
#define Assert(Expression) if (!(Expression)) *(int *)0 = 0;
#endif

#ifndef ZeroMemory
internal void
ZeroMemory(void *Buffer, u32 Size) {
  s8 *Pointer = (i8 *)Buffer;
  while (Size--)
    *(Pointer++) = 0;
}
#endif

#ifndef ArrayCount
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))
#endif

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)

#define MaxBuffers 32
#define MaxPanels 32
#define MaxHistoryStackSize 128
#define BuildBufferName "~~ BUILD RESULT ~~"
#define StartBufferName "~~ Start ~~"

struct rgb {
  u8 Red, Green, Blue;
};

struct theme {
  rgb Background;
  rgb Text;
  rgb Cursor;
  rgb Selection;
  rgb StatusBarBackground;
  rgb StatusBarText;
  rgb InactiveStatusBarBackground;
  rgb InactiveStatusBarText;
  rgb Keyword;
  rgb String;
  rgb Comment;
  rgb Constant;
};

enum buffer_flags {
  BufferFlag_Saved        = 1 << 0,
  BufferFlag_NotEditable  = 1 << 1,
  BufferFlag_NotSavable   = 1 << 2
};

enum line_ending {
  LineEnding_Unix,
  LineEnding_Dos,
  LineEnding_Max
};

enum history_element_type {
  History_Insert,
  History_Remove,
  History_Max
};

struct history_element {
  u32 Type;
  u32 Position;
  u32 Length;
  char *Data;
};

struct buffer {
  char Name[256];
  void *Memory;
  u32 Size;
  u32 Length;
  u32 Flags;
  u32 LineEnding;

  // history
  history_element HistoryStack[MaxHistoryStackSize];
  u32 HistoryStackPosition;
};

global_variable const char *BuiltInCommands[] = {
  "edit", // edit existing buffer or open that file if it's not exists create new buffer
  "jumpto" // jump to line
};

/*enum panel_cmd {
  PanelCmd_Unknown,
  PanelCmd_EditFile,
  PanelCmd_JumpToLine,
  PanelCmd_Max
};*/

global_variable const char *PanelCmdLabels[] = {
  "unknown",
  "edit file",
  "jump to line"
};

// TODO(Brajan): caching
struct panel {
  bool Active;
  buffer *Buffer;
  // NOTE(Brajan): 0-1
  r32 ViewportX;
  r32 ViewportY;
  r32 ViewportWidth;
  r32 ViewportHeight;
  // NOTE(Brajan): in pixels, calculations based
  // on viewport values
  u32 DrawAreaWidth;
  u32 DrawAreaHeight;
  u32 ViewLine;
  u32 CursorPosition;
  u32 CurrentLine;
  bool Selecting;
  u32 SelectStart;
  // command input
  bool ShowCommandPrompt;
  buffer CommandBuffer;
  u32 CommandCursorPosition;
};

enum cmd_type {
  Cmd_NextCharacter,
  Cmd_PrevCharacter,
  Cmd_NextLine,
  Cmd_PrevLine,
  Cmd_RemoveBackward,
  Cmd_RemoveForward,
  Cmd_JumpToBeginOfLine,
  Cmd_JumpToEndOfLine,
  Cmd_WriteBufferToFile,
  Cmd_NextPage,
  Cmd_PrevPage,
  Cmd_ShowEditFilePrompt,
  Cmd_ShowJumpToLinePrompt,
  Cmd_Paste,
  Cmd_Cut,
  Cmd_Copy,
  Cmd_DuplicateLine,
  Cmd_RemoveLine,
  Cmd_CloseAllOtherPanels,
  Cmd_SplitVertically,
  Cmd_SplitHorizontally,
  Cmd_SwitchPanelLeft,
  Cmd_SwitchPanelRight,
  Cmd_SwitchPanelUp,
  Cmd_SwitchPanelDown,
  Cmd_Indent,
  Cmd_NewLine,
  Cmd_CallBuildFile,
  Cmd_NextWord,
  Cmd_PrevWord,
  Cmd_Undo,
  Cmd_Redo,
  Cmd_Max
};

enum async_key_flags {
  AsyncKey_None   = 0,
  AsyncKey_Ctrl   = 1 << 1,
  AsyncKey_Alt    = 1 << 2,
  AsyncKey_Shift  = 1 << 3,
  AsyncKey_Ignore = 1 << 4,
  AsyncKey_Num
};

struct key_binding {
  u8 Key;
  u32 AsyncKeyFlags;  
  void (*Function)(panel *Panel);
};

struct buffer_state {
  bool Loaded;
  buffer Buffer;
};

struct bcoder_state {
  theme Theme;
  u32 LineHeight;
  buffer_state Buffers[MaxBuffers];
  panel Panels[MaxPanels];
  panel *CurrentPanel;

  u32 TabSize;
  bool ShowNumbers;
      
  key_binding KeyBindings[256];
  u32 KeyBindingsNum;
  bool IgnoreNextInput;
};
global_variable bcoder_state BCoderState;

void InitializeBCoder();
void ShutdownBCoder();
void ProcessInput(char Input);
void HandleKeyDown(u32 Key, u32 AsyncKeyFlags);

buffer *GetBuffer(const char *Filename);
void CloseBuffer(buffer *Buffer);

void Bind(u8 Key, u32 AsyncKeyFlags, cmd_type Command);
void BindCustom(u8 Key, u32 AsyncKeyFlags, void (*Function)(panel *Panel));
void CmdNextCharacter(panel *Panel);
void CmdPrevCharacter(panel *Panel);
void CmdNextLine(panel *Panel);
void CmdPrevLine(panel *Panel);
void CmdRemoveBackward(panel *Panel);
void CmdRemoveForward(panel *Panel);
void CmdJumpToBeginOfLine(panel *Panel);
void CmdJumpToEndOfLine(panel *Panel);
void CmdNextPage(panel *Panel);
void CmdPrevPage(panel *Panel);
void CmdWriteBufferToFile(panel *Panel);
void CmdShowEditFilePrompt(panel *Panel);
void CmdShowJumpToLine(panel *Panel);
void CmdPaste(panel *Panel);
void CmdCut(panel *Panel);
void CmdCopy(panel *Panel);
void CmdDuplicateLine(panel *Panel);
void CmdRemoveLine(panel *Panel);
void CmdCloseAllOtherPanels(panel *Panel);
void CmdSplitVertically(panel *Panel);
void CmdSplitHorizontally(panel *Panel);
void CmdSwitchPanelLeft(panel *Panel);
void CmdSwitchPanelRight(panel *Panel);
void CmdSwitchPanelUp(panel *Panel);
void CmdSwitchPanelDown(panel *Panel);
void CmdIndent(panel *Panel);
void CmdNewLine(panel *Panel);
void CmdCallBuildFile(panel *Panel);
void CmdNextWord(panel *Panel);
void CmdPrevWord(panel *Panel);
void CmdUndo(panel *Panel);
void CmdRedo(panel *Panel);

bool IsWordKeyword(const char *Word, u32 Length);
bool IsDelimiter(char Character);
bool IsWhitespace(char Character);
bool IsConstant(const char *Start, u32 Length);

u32 GetMaxLinesOnPanel(u32 Height, u32 LineHeight);
void PanelCursorMoved(panel *Panel);
void PanelViewChanged(panel *Panel);
void PanelShowCommandPrompt(panel *Panel, const char *StartCmdBuffer);
u32 GetPanelCmdByLabel(const char *Label);
bool PanelEditFile(panel *Panel, const char *Filename);
bool PanelJumpToLine(panel *Panel, u32 Line);
u32 GetNumberOfOpenPanels();
void PanelUpdate(panel *Panel);
void ParseAndExecuteOnPanel(panel *Panel);

void PushCharacterToBuffer(buffer *Buffer, char Character);
void PopCharacterFromBuffer(buffer *Buffer);
char GetCharacterAtPosition(buffer *Buffer, u32 Position);
void StringMove(char *Destination, char *Source, u32 Count);
u32 InsertString(buffer *Buffer, u32 Position, char *String, u32 Length);
u32 RemoveCharacters(buffer *Buffer, u32 Position, u32 Num);
void InsertCharacter(buffer *Buffer, u32 Position, char Character);
void RemoveCharacter(buffer *Buffer, u32 Position);
u32 GetPositionWhereLineStarts(buffer *Buffer, u32 Line);
u32 CalculateCursorPositionInLine(buffer *Buffer, u32 Position);
u32 GetLineNumberByPosition(buffer *Buffer, u32 Position);
u32 GetLineLength(buffer *Buffer, u32 Line);
u32 GetLinesNumber(buffer *Buffer);

void AddInsertHistory(buffer *Buffer, u32 Position, u32 Length);

struct platform {
  bool (*ReadFileIntoBuffer)(const char *FileName, buffer *Buffer);
  void (*WriteBufferIntoFile)(buffer* Buffer, const char *FileName);
  void (*ShowError)(const char *Error);
  buffer (*CreateEmptyBuffer)(u32 InitialSize);
  void (*FreeBuffer)(buffer *Buffer);
  char *(*GetSystemClipboard)();
  void (*SetSystemClipboard)(const char *Data, u32 Length);
  void *(*AllocateMemory)(u32 Size);
  void (*FreeMemory)(void *Ptr);
  void (*RecreatePanelsGraphics)();
  void (*LoadSystemFont)(u32 SizeInPixels, const char *FontName);
  u32 (*GetAsyncKeysState)();
  u32 (*RunBuildFile)(const char *Name, char *Buffer, u32 Size, u32 *OutSize);
  void (*Redraw)();
};
global_variable platform Platform;

#define BCODER_H_
#endif

