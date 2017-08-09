#include "bcoder.h"
#include "bcoder_string.h"

const char StartBufferContent[] = {
  "Thanks for using bcoder.\n"
  "C Syntax tests:\n\n\n"
  "\"for\",\n"
  "\"switch\",\n"
};

void
InitializeBCoder() {
  Platform.LoadSystemFont(17, "Liberation Mono");
  
  BCoderState.TabSize = 2;
  BCoderState.ShowNumbers = false;
  BCoderState.CurrentPanel = &BCoderState.Panels[0];
  BCoderState.CurrentPanel->CommandBuffer = Platform.CreateEmptyBuffer(Kilobytes(1));
  BCoderState.CurrentPanel->Buffer = GetBuffer("w:/test.c");
  
#if 0
  BCoderState.CurrentPanel->Buffer = GetBuffer(StartBufferName);
  StringCopy(BCoderState.CurrentPanel->Buffer->Name, StartBufferName);
  StringCopy((char *)BCoderState.CurrentPanel->Buffer->Memory, StartBufferContent);
  BCoderState.CurrentPanel->Buffer->Length = StringLength((char *)&StartBufferContent[0]);
  BCoderState.CurrentPanel->Buffer->Flags = BufferFlag_NotEditable | BufferFlag_NotSavable;
#endif

  BCoderState.CurrentPanel->Buffer->Flags = BufferFlag_Saved;
  BCoderState.CurrentPanel->Active = true;
  BCoderState.CurrentPanel->ViewportX = 0.0f;
  BCoderState.CurrentPanel->ViewportY = 0.0f;
  BCoderState.CurrentPanel->ViewportWidth = 1.0f;
  BCoderState.CurrentPanel->ViewportHeight = 1.0f;
  
  // TODO(Brajan): maybe, in future, i want to load themes from file
  BCoderState.Theme.Background = { 15, 15, 15 };
  BCoderState.Theme.Text = { 149, 134, 116 };
  BCoderState.Theme.Cursor = { 149, 134, 116 };
  BCoderState.Theme.Selection = { 70, 70, 70 };
  BCoderState.Theme.StatusBarBackground = { 149, 134, 116 };
  BCoderState.Theme.StatusBarText = { 15, 15, 15 };
  BCoderState.Theme.InactiveStatusBarBackground = { 95, 86, 74 };
  BCoderState.Theme.InactiveStatusBarText = { 33, 33, 33 };
  BCoderState.Theme.Keyword = { 156, 135, 65 };
  BCoderState.Theme.String = { 93, 105, 55 };
  BCoderState.Theme.Comment = { 61, 61, 61 };
  BCoderState.Theme.Constant = { 193, 147, 83 };

  // key bindings
  Bind(0x26, AsyncKey_Ignore, Cmd_PrevLine);
  Bind(0x28, AsyncKey_Ignore, Cmd_NextLine);
  Bind(0x27, AsyncKey_Ignore, Cmd_NextCharacter);
  Bind(0x25, AsyncKey_Ignore, Cmd_PrevCharacter);
  Bind(0x08, AsyncKey_None, Cmd_RemoveBackward);
  Bind(0x2E, AsyncKey_None, Cmd_RemoveForward);
  Bind(0x24, AsyncKey_Ignore, Cmd_JumpToBeginOfLine);
  Bind(0x23, AsyncKey_Ignore, Cmd_JumpToEndOfLine);
  Bind(0x22, AsyncKey_None, Cmd_NextPage);
  Bind(0x21, AsyncKey_None, Cmd_PrevPage);
  Bind('S', AsyncKey_Ctrl, Cmd_WriteBufferToFile);
  Bind('E', AsyncKey_Ctrl, Cmd_ShowEditFilePrompt);
  Bind('G', AsyncKey_Ctrl, Cmd_ShowJumpToLinePrompt);
  Bind('V', AsyncKey_Ctrl, Cmd_Paste);
  Bind('X', AsyncKey_Ctrl, Cmd_Cut);
  Bind('C', AsyncKey_Ctrl, Cmd_Copy);
  Bind('D', AsyncKey_Ctrl, Cmd_DuplicateLine);
  Bind('D', AsyncKey_Ctrl | AsyncKey_Alt, Cmd_RemoveLine);
  Bind('X', AsyncKey_Ctrl | AsyncKey_Alt, Cmd_CloseAllOtherPanels);
  Bind('V', AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SplitVertically);
  Bind('H', AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SplitHorizontally);
  Bind(0x26, AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SwitchPanelUp);
  Bind(0x28, AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SwitchPanelDown);
  Bind(0x27, AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SwitchPanelRight);
  Bind(0x25, AsyncKey_Ctrl | AsyncKey_Alt, Cmd_SwitchPanelLeft);
  Bind(0x0D, AsyncKey_None, Cmd_NewLine);
  Bind(0x09, AsyncKey_None, Cmd_Indent);
  Bind(0x74, AsyncKey_None, Cmd_CallBuildFile);
  Bind(0x27, AsyncKey_Ctrl, Cmd_NextWord);
  Bind(0x25, AsyncKey_Ctrl, Cmd_PrevWord);
  Bind('Z', AsyncKey_Ctrl, Cmd_Undo);
  Bind('R', AsyncKey_Ctrl, Cmd_Redo);
}

void
ShutdownBCoder() {
  for (u32 Index = 0;
       Index < MaxBuffers;
       ++Index) {
    if (BCoderState.Buffers[Index].Loaded) {    
      BCoderState.Buffers[Index].Loaded = false;
      Platform.FreeBuffer(&BCoderState.Buffers[Index].Buffer);
    }
  }
}

void
ProcessInput(char Input) {
  if (BCoderState.IgnoreNextInput) {
    BCoderState.IgnoreNextInput = false;
    return;
  }

  panel *Panel = BCoderState.CurrentPanel;
  if (!Panel->ShowCommandPrompt) {
    if (Input >= 0x20 && Input <= 0x7E && Input != 0x09) {
      if (!(Panel->Buffer->Flags & BufferFlag_NotEditable)) {
        if (Panel->Selecting) {
          u32 SelectStart = 0;
          u32 SelectEnd = 0;
          if (Panel->SelectStart > Panel->CursorPosition) {
            SelectStart = Panel->CursorPosition;
            SelectEnd = Panel->SelectStart;
          } else {
            SelectStart = Panel->SelectStart;
            SelectEnd = Panel->CursorPosition;
          }
          u32 Length = SelectEnd - SelectStart;

          u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
          if (Panel->SelectStart < Panel->CursorPosition) {
            Panel->CursorPosition -= RemovedCharacters;
          }
          Panel->Selecting = false;
        }
        InsertCharacter(Panel->Buffer, Panel->CursorPosition++, Input);
        AddInsertHistory(Panel->Buffer, Panel->CursorPosition - 1, 1);

        PanelCursorMoved(Panel);
        PanelUpdate(Panel);
      }
    }
  } else {
    if (Input >= 0x20 && Input <= 0x7E) {
      InsertCharacter(&Panel->CommandBuffer, Panel->CommandCursorPosition++, Input);
      PanelCursorMoved(Panel);
      PanelUpdate(Panel);
    }
  }
}

void
HandleKeyDown(u32 Key, u32 AsyncKeyFlags) {
  panel *Panel = BCoderState.CurrentPanel;

  if (Panel->ShowCommandPrompt) {
    if (Key == 0x1B) { // escape
      Panel->ShowCommandPrompt = false;
      PanelUpdate(Panel);
    } else if (Key == 0x0D) { // return/enter key
      ParseAndExecuteOnPanel(Panel);
      BCoderState.IgnoreNextInput = true;
      Panel->ShowCommandPrompt = false;
      PanelUpdate(Panel);
    }
  }
  
  for (u32 Index = 0;
       Index < BCoderState.KeyBindingsNum;
       ++Index) {
    key_binding *KeyBinding = &BCoderState.KeyBindings[Index];
    if (KeyBinding->Key == Key) {
      if (KeyBinding->AsyncKeyFlags != AsyncKey_Ignore && KeyBinding->AsyncKeyFlags != AsyncKeyFlags)
        continue;

      if (KeyBinding->Function)
        KeyBinding->Function(Panel);
      PanelUpdate(Panel);
    }
  }
}

buffer *
GetBuffer(const char *Name) {
  for (u32 Index = 0;
       Index < MaxBuffers;
       ++Index) {
    if (StringCompare(BCoderState.Buffers[Index].Buffer.Name, Name) == 0) {
      return &BCoderState.Buffers[Index].Buffer;
    }
  }

  buffer_state *BufferState = 0;
  buffer *Buffer = 0;
  for (u32 Index = 0;
       Index < MaxBuffers;
       ++Index) {
    if (!BCoderState.Buffers[Index].Loaded) {
      BufferState = &BCoderState.Buffers[Index];
      Buffer = &BCoderState.Buffers[Index].Buffer;
      break;
    }
  }

  if (Buffer != 0) {
    if (Platform.ReadFileIntoBuffer(Name, Buffer)) {
      StringCopy(Buffer->Name, Name);
      BufferState->Loaded = true;
    } else {
      *Buffer = Platform.CreateEmptyBuffer(Megabytes(1));
      StringCopy(Buffer->Name, Name);
      InsertCharacter(Buffer, 0, '\n');
      BufferState->Loaded = true;
    }
  }
  return Buffer;
}

void
CloseBuffer(buffer *Buffer) {
  // NOTE(Brajan): I'm checking here if any panels is using this buffer
  // if not, I can feel free to free it.
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (BCoderState.Panels[Index].Buffer == Buffer)
      return;
  }

  Platform.FreeBuffer(Buffer);
  for (u32 Index = 0;
       Index < MaxBuffers;
       ++Index) {
    if (&BCoderState.Buffers[Index].Buffer == Buffer) {
      BCoderState.Buffers[Index].Loaded = false;
      ZeroMemory(&BCoderState.Buffers[Index], sizeof(buffer));
      break;
    }
  }
}

void
Bind(u8 Key, u32 AsyncKeyFlags, cmd_type Command) {
  key_binding *KeyBinding = &BCoderState.KeyBindings[BCoderState.KeyBindingsNum++];
  KeyBinding->Key = Key;
  KeyBinding->AsyncKeyFlags = AsyncKeyFlags;
  switch (Command) {
    case Cmd_NextCharacter:
      KeyBinding->Function = CmdNextCharacter;
      break;
    case Cmd_PrevCharacter:
      KeyBinding->Function = CmdPrevCharacter;
      break;
    case Cmd_NextLine:
      KeyBinding->Function = CmdNextLine;
      break;
    case Cmd_PrevLine:
      KeyBinding->Function = CmdPrevLine;
      break;
    case Cmd_RemoveBackward:
      KeyBinding->Function = CmdRemoveBackward;
      break;
    case Cmd_RemoveForward:
      KeyBinding->Function = CmdRemoveForward;
      break;
    case Cmd_JumpToBeginOfLine:
      KeyBinding->Function = CmdJumpToBeginOfLine;
      break;
    case Cmd_JumpToEndOfLine:
      KeyBinding->Function = CmdJumpToEndOfLine;
      break;
    case Cmd_WriteBufferToFile:
      KeyBinding->Function = CmdWriteBufferToFile;
      break;
    case Cmd_NextPage:
      KeyBinding->Function = CmdNextPage;
      break;
    case Cmd_PrevPage:
      KeyBinding->Function = CmdPrevPage;
      break;
    case Cmd_ShowEditFilePrompt:
      KeyBinding->Function = CmdShowEditFilePrompt;
      break;
    case Cmd_ShowJumpToLinePrompt:
      KeyBinding->Function = CmdShowJumpToLine;
      break;
    case Cmd_Paste:
      KeyBinding->Function = CmdPaste;
      break;
    case Cmd_Cut:
      KeyBinding->Function = CmdCut;
      break;
    case Cmd_Copy:
      KeyBinding->Function = CmdCopy;
      break;
    case Cmd_DuplicateLine:
      KeyBinding->Function = CmdDuplicateLine;
      break;
    case Cmd_RemoveLine:
      KeyBinding->Function = CmdRemoveLine;
      break;
    case Cmd_CloseAllOtherPanels:
      KeyBinding->Function = CmdCloseAllOtherPanels;
      break;
    case Cmd_SplitVertically:
      KeyBinding->Function = CmdSplitVertically;
      break;
    case Cmd_SplitHorizontally:
      KeyBinding->Function = CmdSplitHorizontally;
      break;
    case Cmd_SwitchPanelLeft:
      KeyBinding->Function = CmdSwitchPanelLeft;
      break;
    case Cmd_SwitchPanelRight:
      KeyBinding->Function = CmdSwitchPanelRight;
      break;
    case Cmd_SwitchPanelUp:
      KeyBinding->Function = CmdSwitchPanelUp;
      break;
    case Cmd_SwitchPanelDown:
      KeyBinding->Function = CmdSwitchPanelDown;
      break;
    case Cmd_Indent:
      KeyBinding->Function = CmdIndent;
      break;
    case Cmd_NewLine:
      KeyBinding->Function = CmdNewLine;
      break;
    case Cmd_CallBuildFile:
      KeyBinding->Function = CmdCallBuildFile;
      break;
    case Cmd_NextWord:
      KeyBinding->Function = CmdNextWord;
      break;
    case Cmd_PrevWord:
      KeyBinding->Function = CmdPrevWord;
      break;
    case Cmd_Undo:
      KeyBinding->Function = CmdUndo;
      break;
    case Cmd_Redo:
      KeyBinding->Function = CmdRedo;
      break;
    default:
      Assert(false); 
      break;
  }
}

void
BindCustom(u8 Key, u32 AsyncKeyFlags, void (*Function)(panel *Panel)) {
  key_binding *KeyBinding = &BCoderState.KeyBindings[BCoderState.KeyBindingsNum++];
  KeyBinding->Key = Key;
  KeyBinding->AsyncKeyFlags = AsyncKeyFlags;
  KeyBinding->Function = Function;
}

void
CmdNextCharacter(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
    if (AsyncKeysFlags == AsyncKey_Shift) {
      if (!Panel->Selecting) {
        Panel->Selecting = true;
        Panel->SelectStart = Panel->CursorPosition;
      }
    } else {
      Panel->Selecting = false;
      if (AsyncKeysFlags != AsyncKey_None) {
        return;
      }
    }

    u32 CurrentLine = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
    u32 PositionInLine = CalculateCursorPositionInLine(Panel->Buffer, Panel->CursorPosition);
    u32 LineLength = GetLineLength(Panel->Buffer, CurrentLine);

    if (PositionInLine >= LineLength)
      return;
    
    char *Buffer = (char *)Panel->Buffer->Memory;
    if (*(Buffer + Panel->CursorPosition) == '\r' && Panel->CursorPosition < Panel->Buffer->Length) {
      return;
    }

    if (Panel->CursorPosition < Panel->Buffer->Length) {
      ++Panel->CursorPosition;
    }
  } else {
    if (Panel->CommandCursorPosition < Panel->CommandBuffer.Length) {
      ++Panel->CommandCursorPosition;
    }
    Panel->Selecting = false;
  }
  PanelCursorMoved(Panel);
}

void
CmdPrevCharacter(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
    if (AsyncKeysFlags == AsyncKey_Shift) {
      if (!Panel->Selecting) {
        Panel->Selecting = true;
        Panel->SelectStart = Panel->CursorPosition;
      }
    } else {
      Panel->Selecting = false;
      if (AsyncKeysFlags != AsyncKey_None) {
        return;
      }
    }

    u32 PositionInLine = CalculateCursorPositionInLine(Panel->Buffer, Panel->CursorPosition);
    if (PositionInLine == 0)
      return;

    char *Buffer = (char *)Panel->Buffer->Memory;
    if (*(Buffer + Panel->CursorPosition) == '\r') {
      //--Panel->CursorPosition;
      //return;
    }

    if (Panel->CursorPosition > 0) {
      --Panel->CursorPosition;
    }
  } else {
    // NOTE(Brajan): I dont support multiline command input
    if (Panel->CommandCursorPosition > 0) {
      --Panel->CommandCursorPosition;
    }
    Panel->Selecting = false;
  }
  PanelCursorMoved(Panel);
}

void
CmdNextLine(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
  if (AsyncKeysFlags == AsyncKey_Shift) {
    if (!Panel->Selecting) {
      Panel->Selecting = true;
      Panel->SelectStart = Panel->CursorPosition;
    }
  } else {
    Panel->Selecting = false;
    if (AsyncKeysFlags != AsyncKey_None) {
      return;
    }
  }

  buffer *Buffer = Panel->Buffer;
  u32 Line = GetLineNumberByPosition(Buffer, Panel->CursorPosition);
  u32 LinePosition = CalculateCursorPositionInLine(Buffer, Panel->CursorPosition);
  
  if (GetPositionWhereLineStarts(Buffer, Line) + GetLineLength(Buffer, Line) == Buffer->Length - 1)
    return;

  u32 NextLineStartPosition = GetPositionWhereLineStarts(Buffer, Line + 1);
  u32 NextLineLength = GetLineLength(Buffer, Line + 1);
  if (Panel->CursorPosition == 0) {
    Panel->CursorPosition = NextLineStartPosition;
  } else if (NextLineLength == 0) {
    Panel->CursorPosition = NextLineStartPosition;
  } else if (LinePosition > NextLineLength) {
    Panel->CursorPosition = NextLineStartPosition + NextLineLength;
  } else {
    Panel->CursorPosition = NextLineStartPosition + LinePosition;
  }
  PanelCursorMoved(Panel);
}

void
CmdPrevLine(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
  if (AsyncKeysFlags == AsyncKey_Shift) {
    if (!Panel->Selecting) {
      Panel->Selecting = true;
      Panel->SelectStart = Panel->CursorPosition;
    }
  } else {
    Panel->Selecting = false;
    if (AsyncKeysFlags != AsyncKey_None) {
      return;
    }
  }

  buffer *Buffer = Panel->Buffer;
  u32 Line = GetLineNumberByPosition(Buffer, Panel->CursorPosition);
  if (Line > 0) {
    u32 LinePosition = CalculateCursorPositionInLine(Buffer, Panel->CursorPosition);
    u32 PrevLineStartPosition = GetPositionWhereLineStarts(Buffer, Line - 1);
    u32 PrevLineLength = GetLineLength(Buffer, Line - 1);
    
    if (PrevLineLength == 0) {
      Panel->CursorPosition = PrevLineStartPosition;
    } else if (LinePosition > PrevLineLength) {
      Panel->CursorPosition = PrevLineStartPosition + PrevLineLength;
    } else {
      Panel->CursorPosition = PrevLineStartPosition + LinePosition;
    }
  }
  PanelCursorMoved(Panel);
}

void
CmdRemoveBackward(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
      return;
    }

    if (Panel->Selecting) {
      u32 SelectStart = 0;
      u32 SelectEnd = 0;
      if (Panel->SelectStart > Panel->CursorPosition) {
        SelectStart = Panel->CursorPosition;
        SelectEnd = Panel->SelectStart;
      } else {
        SelectStart = Panel->SelectStart;
        SelectEnd = Panel->CursorPosition;
      }
      u32 Length = SelectEnd - SelectStart;

      u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
      if (Panel->SelectStart < Panel->CursorPosition) {
        Panel->CursorPosition -= RemovedCharacters;
      }
      Panel->Selecting = false;
    } else {
      if (Panel->CursorPosition > 0) {
        char *Buffer = (char *)Panel->Buffer->Memory;

        if (*(Buffer + Panel->CursorPosition) == '\r') {
          if (*(Buffer + Panel->CursorPosition + 1) == '\n')
            RemoveCharacter(Panel->Buffer, Panel->CursorPosition + 1);
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition--);
        } else if (*(Buffer + Panel->CursorPosition) == '\n' && Panel->CursorPosition > 1) {
          if (*(Buffer + Panel->CursorPosition - 1) == '\r')
            RemoveCharacter(Panel->Buffer, Panel->CursorPosition--);
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition--);
        } else {
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition--);
        }
      }
    }
  } else {
    if (Panel->CommandCursorPosition > 0)
      RemoveCharacter(&Panel->CommandBuffer, Panel->CommandCursorPosition--);
  }
  PanelCursorMoved(Panel);
}

void
CmdRemoveForward(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
      return;
    }
    if (Panel->Selecting) {
      u32 SelectStart = 0;
      u32 SelectEnd = 0;
      if (Panel->SelectStart > Panel->CursorPosition) {
        SelectStart = Panel->CursorPosition;
        SelectEnd = Panel->SelectStart;
      } else {
        SelectStart = Panel->SelectStart;
        SelectEnd = Panel->CursorPosition;
      }
      u32 Length = SelectEnd - SelectStart;

      u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
      if (Panel->SelectStart < Panel->CursorPosition) {
        Panel->CursorPosition -= RemovedCharacters;
      }
      Panel->Selecting = false;
    } else {
      if (Panel->CursorPosition + 1 < Panel->Buffer->Length) {
        char *Buffer = (char *)Panel->Buffer->Memory;

        if (*(Buffer + Panel->CursorPosition) == '\r') {
          if (*(Buffer + Panel->CursorPosition + 1) == '\n')
            RemoveCharacter(Panel->Buffer, Panel->CursorPosition + 1);
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition + 1);
        } else if (*(Buffer + Panel->CursorPosition) == '\n' && Panel->CursorPosition > 1) {
          if (*(Buffer + Panel->CursorPosition - 1) == '\r')
            RemoveCharacter(Panel->Buffer, Panel->CursorPosition - 1);
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition);
        } else {
          RemoveCharacter(Panel->Buffer, Panel->CursorPosition + 1);
        }
      }
    }
  } else {
    if (Panel->CommandCursorPosition + 1 < Panel->CommandBuffer.Length)
      RemoveCharacter(&Panel->CommandBuffer, Panel->CommandCursorPosition + 1);
  }
  PanelCursorMoved(Panel);
}

void
CmdJumpToBeginOfLine(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
    if (AsyncKeysFlags == AsyncKey_Shift) {
      if (!Panel->Selecting) {
        Panel->Selecting = true;
        Panel->SelectStart = Panel->CursorPosition;
      }
    } else {
      Panel->Selecting = false;
      if (AsyncKeysFlags != AsyncKey_None) {
        return;
      }
    }
    Panel->CursorPosition = GetPositionWhereLineStarts(Panel->Buffer, GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition));
  } else {
    Panel->CommandCursorPosition = 0;
  }
  PanelCursorMoved(Panel);
}

void
CmdJumpToEndOfLine(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    u32 AsyncKeysFlags = Platform.GetAsyncKeysState();
    if (AsyncKeysFlags == AsyncKey_Shift) {
      if (!Panel->Selecting) {
        Panel->Selecting = true;
        Panel->SelectStart = Panel->CursorPosition;
      }
    } else {
      Panel->Selecting = false;
      if (AsyncKeysFlags != AsyncKey_None) {
        return;
      }
    }
    u32 Line = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
    Panel->CursorPosition = GetPositionWhereLineStarts(Panel->Buffer, Line) + GetLineLength(Panel->Buffer, Line);
  } else {
    Panel->CommandCursorPosition = Panel->CommandBuffer.Length;
  }
  PanelCursorMoved(Panel);
}

void
CmdNextPage(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  u32 LinesOnPanel = GetMaxLinesOnPanel(Panel->DrawAreaHeight, BCoderState.LineHeight);
  u32 LastLine = GetLinesNumber(Panel->Buffer) - 1;
  if (Panel->ViewLine + LinesOnPanel < LastLine)
    Panel->ViewLine += LinesOnPanel;
  else
    Panel->ViewLine = LastLine;
  PanelViewChanged(Panel);
}

void
CmdPrevPage(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  u32 LinesOnPanel = GetMaxLinesOnPanel(Panel->DrawAreaHeight, BCoderState.LineHeight);
  if ((s32)Panel->ViewLine - (s32)LinesOnPanel > 0)
    Panel->ViewLine -= LinesOnPanel;
  else
    Panel->ViewLine = 0;
  PanelViewChanged(Panel);
}

void
CmdWriteBufferToFile(panel *Panel) {
  Platform.WriteBufferIntoFile(BCoderState.CurrentPanel->Buffer, BCoderState.CurrentPanel->Buffer->Name);
}

void
CmdShowEditFilePrompt(panel *Panel) {
  PanelShowCommandPrompt(Panel, "edit ");
}

void
CmdShowJumpToLine(panel *Panel) {
  PanelShowCommandPrompt(Panel, "jumpto ");
}

void
CmdPaste(panel *Panel) {
  if (!Panel->ShowCommandPrompt) {
    if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
      return;
    }

    if (Panel->Selecting) {
      u32 SelectStart = 0;
      u32 SelectEnd = 0;
      if (Panel->SelectStart > Panel->CursorPosition) {
        SelectStart = Panel->CursorPosition;
        SelectEnd = Panel->SelectStart;
      } else {
        SelectStart = Panel->SelectStart;
        SelectEnd = Panel->CursorPosition;
      }

      u32 Length = SelectEnd - SelectStart;
      u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
      if (Panel->SelectStart < Panel->CursorPosition) {
        Panel->CursorPosition -= RemovedCharacters;
      }
      Panel->Selecting = false;
    }

    u32 BeginPosition = Panel->CursorPosition;

    char *Clipboard = Platform.GetSystemClipboard();
    u32 ClipboardLength = StringLength(Clipboard);
    if (Clipboard != 0 && ClipboardLength > 0) {
      if (Panel->Buffer->LineEnding == LineEnding_Dos) {
        bool WasCR = false;
        for (u32 Index = 0;
             Index < ClipboardLength;
             ++Index) {
          if (Clipboard[Index] == '\r') {
            WasCR = true;
          } else if (Clipboard[Index] == '\n' && !WasCR) {
            InsertCharacter(Panel->Buffer, Panel->CursorPosition++, '\r');
            InsertCharacter(Panel->Buffer, Panel->CursorPosition++, '\n');
          } else {
            InsertCharacter(Panel->Buffer, Panel->CursorPosition++, Clipboard[Index]);
            WasCR = false;
          }
        }
      } else if (Panel->Buffer->LineEnding == LineEnding_Unix) {
        for (u32 Index = 0;
             Index < ClipboardLength;
             ++Index) {
          if (Clipboard[Index] == '\r')
            continue;
          InsertCharacter(Panel->Buffer, Panel->CursorPosition++, Clipboard[Index]);
        }
      }
      Platform.FreeMemory(Clipboard);
      AddInsertHistory(Panel->Buffer, BeginPosition, Panel->CursorPosition - BeginPosition);
    }
  } else {
    char *Clipboard = Platform.GetSystemClipboard();
    if (Clipboard != 0) {
      Panel->CommandCursorPosition += InsertString(&Panel->CommandBuffer, Panel->CommandCursorPosition, Clipboard, StringLength(Clipboard));
      Platform.FreeMemory(Clipboard);
    }
  }
  PanelCursorMoved(Panel);
}

void
CmdCut(panel *Panel) {
  if (!Panel->Selecting)
    return;
  if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  u32 SelectStart = 0;
  u32 SelectEnd = 0;
  if (Panel->SelectStart > Panel->CursorPosition) {
    SelectStart = Panel->CursorPosition;
    SelectEnd = Panel->SelectStart;
  } else {
    SelectStart = Panel->SelectStart;
    SelectEnd = Panel->CursorPosition;
  }

  char *Begin = (char *)Panel->Buffer->Memory + SelectStart;
  u32 Length = SelectEnd - SelectStart;
  Platform.SetSystemClipboard(Begin, Length);

  u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
  if (Panel->SelectStart < Panel->CursorPosition) {
    Panel->CursorPosition -= RemovedCharacters;
  }
  Panel->Selecting = false;
  PanelCursorMoved(Panel);
}

void
CmdCopy(panel *Panel) {
  if (!Panel->Selecting)
    return;
  u32 SelectStart = 0;
  u32 SelectEnd = 0;
  if (Panel->SelectStart > Panel->CursorPosition) {
    SelectStart = Panel->CursorPosition;
    SelectEnd = Panel->SelectStart;
  } else {
    SelectStart = Panel->SelectStart;
    SelectEnd = Panel->CursorPosition;
  }

  char *Begin = (char *)Panel->Buffer->Memory + SelectStart;
  u32 Length = SelectEnd - SelectStart;
  Platform.SetSystemClipboard(Begin, Length);
  PanelCursorMoved(Panel);
}

void
CmdDuplicateLine(panel* Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  u32 Line = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
  u32 LineStart = GetPositionWhereLineStarts(Panel->Buffer, Line);
  u32 LineLength = GetLineLength(Panel->Buffer, Line);
  InsertString(Panel->Buffer, LineStart, (char *)Panel->Buffer->Memory + LineStart, LineLength);
  InsertCharacter(Panel->Buffer, LineStart + LineLength, '\n');
  PanelCursorMoved(Panel);
}

void
CmdRemoveLine(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;
  if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }
 
  u32 Line = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
  u32 LineStart = GetPositionWhereLineStarts(Panel->Buffer, Line);
  u32 LineLength = GetLineLength(Panel->Buffer, Line);
  RemoveCharacters(Panel->Buffer, LineStart, LineLength + 1);
  PanelCursorMoved(Panel);
}

void
CmdCloseAllOtherPanels(panel *Panel) {
  Panel->ViewportX = 0.0f;
  Panel->ViewportY = 0.0f;
  Panel->ViewportWidth = 1.0f;
  Panel->ViewportHeight = 1.0f;

  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (&BCoderState.Panels[Index] == Panel)
     continue;
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    BCoderState.Panels[Index].Active = false;
    buffer *Buffer = BCoderState.Panels[Index].Buffer;
    ZeroMemory(&BCoderState.Panels[Index], sizeof(panel));
    CloseBuffer(Buffer);
  }
  Platform.RecreatePanelsGraphics();
}

void
CmdSplitVertically(panel *Panel) {
  panel *SplittedPanel = 0;
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active) {
      SplittedPanel = &BCoderState.Panels[Index];
      break;
    }
  }
  if (SplittedPanel == 0)
    Assert(false);

  Panel->ViewportWidth *= 0.5f;

  SplittedPanel->Active = true;
  SplittedPanel->Buffer = Panel->Buffer;
  SplittedPanel->ViewportX = Panel->ViewportX + Panel->ViewportWidth;
  SplittedPanel->ViewportY = Panel->ViewportY;
  SplittedPanel->ViewportWidth = Panel->ViewportWidth;
  SplittedPanel->ViewportHeight = Panel->ViewportHeight;

  Platform.RecreatePanelsGraphics();
}

void
CmdSplitHorizontally(panel *Panel) {
  panel *SplittedPanel = 0;
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active) {
      SplittedPanel = &BCoderState.Panels[Index];
      break;
    }
  }
  if (SplittedPanel == 0)
    Assert(false);

  Panel->ViewportHeight *= 0.5f;

  SplittedPanel->Active = true;
  SplittedPanel->Buffer = Panel->Buffer;
  SplittedPanel->ViewportX = Panel->ViewportX;
  SplittedPanel->ViewportY = Panel->ViewportY + Panel->ViewportHeight;
  SplittedPanel->ViewportWidth = Panel->ViewportWidth;
  SplittedPanel->ViewportHeight = Panel->ViewportHeight;

  Platform.RecreatePanelsGraphics();
}

void
CmdSwitchPanelLeft(panel *Panel) {
  if (Panel->ViewportX == 0.0f)
    return;

  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    if (BCoderState.Panels[Index].ViewportX + BCoderState.Panels[Index].ViewportWidth == Panel->ViewportX &&
        BCoderState.Panels[Index].ViewportY <= Panel->ViewportY &&
        BCoderState.Panels[Index].ViewportY + BCoderState.Panels[Index].ViewportHeight > Panel->ViewportY) {
      BCoderState.CurrentPanel = &BCoderState.Panels[Index];
    }
  }
}

void
CmdSwitchPanelRight(panel *Panel) {
  if (Panel->ViewportWidth == 1.0f)
    return;
  
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    if (BCoderState.Panels[Index].ViewportX == Panel->ViewportX + Panel->ViewportWidth &&
        BCoderState.Panels[Index].ViewportY <= Panel->ViewportY &&
        BCoderState.Panels[Index].ViewportY + BCoderState.Panels[Index].ViewportHeight > Panel->ViewportY) {
      BCoderState.CurrentPanel = &BCoderState.Panels[Index];
    }
  }
}

void
CmdSwitchPanelUp(panel *Panel) {
  if (Panel->ViewportY == 0.0f)
    return;

  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    if (BCoderState.Panels[Index].ViewportX <= Panel->ViewportX &&
        BCoderState.Panels[Index].ViewportX + BCoderState.Panels[Index].ViewportWidth > Panel->ViewportX &&
        BCoderState.Panels[Index].ViewportY + BCoderState.Panels[Index].ViewportHeight == Panel->ViewportY) {
      BCoderState.CurrentPanel = &BCoderState.Panels[Index];
    }
  }
}

void
CmdSwitchPanelDown(panel *Panel) {
  if (Panel->ViewportHeight == 1.0f)
    return;
  
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    if (BCoderState.Panels[Index].ViewportX <= Panel->ViewportX &&
        BCoderState.Panels[Index].ViewportX + BCoderState.Panels[Index].ViewportWidth > Panel->ViewportX &&
        BCoderState.Panels[Index].ViewportY == Panel->ViewportY + Panel->ViewportHeight) {
      BCoderState.CurrentPanel = &BCoderState.Panels[Index];
    }
  }
}

void
CmdIndent(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;

  if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  if (Panel->Selecting) {
    u32 SelectStart = 0;
    u32 SelectEnd = 0;
    if (Panel->SelectStart > Panel->CursorPosition) {
      SelectStart = Panel->CursorPosition;
      SelectEnd = Panel->SelectStart;
    } else {
      SelectStart = Panel->SelectStart;
      SelectEnd = Panel->CursorPosition;
    }
    u32 Length = SelectEnd - SelectStart;

    u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
    if (Panel->SelectStart < Panel->CursorPosition) {
      Panel->CursorPosition -= RemovedCharacters;
    }
    Panel->Selecting = false;
  }

  for (u32 Iteration = 0;
       Iteration < BCoderState.TabSize;
       ++Iteration) {
    InsertCharacter(Panel->Buffer, Panel->CursorPosition++, ' ');
  }
  PanelCursorMoved(Panel);
}

void
CmdNewLine(panel *Panel) {
  if (Panel->ShowCommandPrompt)
    return;

  if (Panel->Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  if (Panel->Selecting) {
    u32 SelectStart = 0;
    u32 SelectEnd = 0;
    if (Panel->SelectStart > Panel->CursorPosition) {
      SelectStart = Panel->CursorPosition;
      SelectEnd = Panel->SelectStart;
    } else {
      SelectStart = Panel->SelectStart;
      SelectEnd = Panel->CursorPosition;
    }
    u32 Length = SelectEnd - SelectStart;

    u32 RemovedCharacters = RemoveCharacters(Panel->Buffer, SelectStart, Length);
    if (Panel->SelectStart < Panel->CursorPosition) {
      Panel->CursorPosition -= RemovedCharacters;
    }
    Panel->Selecting = false;
  }

  if (BCoderState.IgnoreNextInput) {
    BCoderState.IgnoreNextInput = false;
    return;
  }

  u32 Line = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
  u32 LineStart = GetPositionWhereLineStarts(Panel->Buffer, Line);
  u32 LineLength = GetLineLength(Panel->Buffer, Line);
  u32 SpaceCounter = 0;
  for (u32 Index = 0;
       Index < LineLength;
       ++Index) {
    if (*((char *)Panel->Buffer->Memory + LineStart + Index) != ' ')
      break;
    ++SpaceCounter;
  }

  if (Panel->Buffer->LineEnding == LineEnding_Dos)
    InsertCharacter(Panel->Buffer, Panel->CursorPosition++, '\r');
  InsertCharacter(Panel->Buffer, Panel->CursorPosition++, '\n');
  for (u32 Iteration = 0;
       Iteration < SpaceCounter;
       ++Iteration) {
    InsertCharacter(Panel->Buffer, Panel->CursorPosition++, ' ');
  }
  PanelCursorMoved(Panel);
}

void
CmdCallBuildFile(panel *Panel) {
  buffer *Buffer = 0;
  for (u32 Index = 0;
       Index < MaxBuffers;
       ++Index) {
    if (!BCoderState.Buffers[Index].Loaded)
      continue;

    if (StringCompare(BCoderState.Buffers[Index].Buffer.Name, BuildBufferName) == 0) {
      Buffer = &BCoderState.Buffers[Index].Buffer;
      break;
    }
  }

  if (Buffer == 0) {
    u32 NumPanels = GetNumberOfOpenPanels();
    panel *BuildPanel = 0;

    if (NumPanels == 2) {
      for (u32 Index = 0;
           Index < MaxPanels;
           ++Index) {
        if (!BCoderState.Panels[Index].Active)
          continue;
        if (&BCoderState.Panels[Index] == Panel)
          continue;
        
        BuildPanel = &BCoderState.Panels[Index];
      }
    } else {
      CmdSplitVertically(Panel);
      for (u32 Index = 0;
           Index < MaxPanels;
           ++Index) {
        if (!BCoderState.Panels[Index].Active)
          continue;
        
        if (BCoderState.Panels[Index].ViewportX == Panel->ViewportX + Panel->ViewportWidth &&
            BCoderState.Panels[Index].ViewportY <= Panel->ViewportY &&
            BCoderState.Panels[Index].ViewportY + BCoderState.Panels[Index].ViewportHeight > Panel->ViewportY) {
          BuildPanel = &BCoderState.Panels[Index];
        }
      }
    }
    PanelEditFile(BuildPanel, BuildBufferName);
    Buffer = BuildPanel->Buffer;
  }
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (BCoderState.Panels[Index].Buffer == Buffer)
      BCoderState.Panels[Index].CursorPosition = 0;
  }

  Buffer->Flags |= BufferFlag_NotEditable;
  Buffer->Flags |= BufferFlag_NotSavable;

  u32 WrittenDataLength = 0;
  u32 Result = Platform.RunBuildFile("build", (char *)Buffer->Memory, Megabytes(1), &WrittenDataLength);
  if (Result != 0 || WrittenDataLength == 0)
    return;

  Buffer->Length = WrittenDataLength;
}

void
CmdNextWord(panel *Panel) {
  char *Buffer = 0;
  u32 *CursorPosition = 0;
  u32 Length = 0;

  if (!Panel->ShowCommandPrompt) {
    Buffer = (char *)Panel->Buffer->Memory;
    CursorPosition = &Panel->CursorPosition;
    Length = Panel->Buffer->Length;
  } else {
    Buffer = (char *)Panel->CommandBuffer.Memory;
    CursorPosition = &Panel->CommandCursorPosition;
    Length = Panel->CommandBuffer.Length;
  }

  bool StartsFromWord = false;
  if (!IsWhitespace(*(Buffer + *CursorPosition)))
    StartsFromWord = true;

  for (;;) {
    if (*CursorPosition >= Length) {
      *CursorPosition = Length;
      break;
    }

    if (StartsFromWord) {
      if (IsWhitespace(*(Buffer + *CursorPosition))) {
        StartsFromWord = false;
      }
    } else {
      if (!IsWhitespace(*(Buffer + *CursorPosition))) {
        break;
      }
    }
    ++(*CursorPosition);
  }
}

void
CmdPrevWord(panel *Panel) {
  char *Buffer = 0;
  u32 *CursorPosition = 0;
  u32 Length = 0;

  if (!Panel->ShowCommandPrompt) {
    Buffer = (char *)Panel->Buffer->Memory;
    CursorPosition = &Panel->CursorPosition;
    Length = Panel->Buffer->Length;
  } else {
    Buffer = (char *)Panel->CommandBuffer.Memory;
    CursorPosition = &Panel->CommandCursorPosition;
    Length = Panel->CommandBuffer.Length;
  }

  bool StartsFromWord = false;
  if (IsLetter(*(Buffer + *CursorPosition)))
    StartsFromWord = true;

  bool FoundWord = false;

  for (;;) {
    if (*CursorPosition == 0) {
      *CursorPosition = 0;
      break;
    }

    if (StartsFromWord) {
      if (IsWhitespace(*(Buffer + *CursorPosition))) {
        StartsFromWord = false;
      }
    } else {
      if (!IsWhitespace(*(Buffer + *CursorPosition)) && !FoundWord) {
        FoundWord = true;
      } else if (FoundWord) {
        if (IsWhitespace(*(Buffer + *CursorPosition))) {
          ++(*CursorPosition);
          break;
        }
      }
    }
    --(*CursorPosition);
  }
}

void
CmdUndo(panel *Panel) {
  if (Panel->ShowCommandPrompt) {
    return;
  }
  
  if (Panel->Buffer->HistoryStackPosition == 0) {
    return;
  }

  history_element *Element = &Panel->Buffer->HistoryStack[--Panel->Buffer->HistoryStackPosition];
  if (Element->Type == History_Insert) {
    RemoveCharacters(Panel->Buffer, Element->Position, Element->Length);
    Panel->CursorPosition = Element->Position;
  }
}

void
CmdRedo(panel *Panel) {
  if (Panel->ShowCommandPrompt) {
    return;
  }
  // ...
}

// TODO(Brajan): make syntax higlighting for more languages
static char Keywords[][24] = {
  "alignas",
  "alignof",
  "and",
  "and_eq",
  "asm",
  "auto",
  "bitand",
  "bitor",
  "bool",
  "break",
  "case",
  "catch",
  "char",
  "char16_t",
  "char32_t",
  "class",
  "compl",
  "const",
  "constexpr",
  "const_cast",
  "continue",
  "decltype",
  "default",
  "delete",
  "do",
  "double",
  "dynamic_cast",
  "else",
  "enum",
  "explicit",
  "export",
  "extern",
  "false",
  "false",
  "float",
  "for",
  "friend",
  "goto",
  "if",
  "inline",
  "int",
  "long",
  "mutable",
  "namespace",
  "new",
  "noexcept",
  "not",
  "not_eq",
  "nullptr",
  "operator",
  "or",
  "or_eq",
  "private",
  "protected",
  "public",
  "register",
  "reinterpret_cast",
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "static_assert",
  "static_cast",
  "struct",
  "switch",
  "template",
  "this",
  "thread_local",
  "throw",
  "true",
  "try",
  "typedef",
  "typeid",
  "typename",
  "union",
  "unsigned",
  "using",
  "virtual",
  "volatile",
  "void",
  "wchar_t",
  "while",
  "xor",
  "xor_eq"
};

bool
IsWordKeyword(const char *Word, u32 Length) {
  for (u32 Index = 0;
       Index < ArrayCount(Keywords);
       ++Index) {
    if (Length == StringLength(Keywords[Index])) {
      if (StringCompareLength(Word, Keywords[Index], Length) == 0)
        return true;
    }
  }
  return false;
}

// TODO(Brajan): rename that maybe to IsDelimiter? I dont know, need to think about that
bool
IsDelimiter(char Character) {
  switch (Character) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '=':
    case '%':
    case '!':
    case '>':
    case '<':
    case '&':
    case '|':
    case '~':
    case '^':
    case '[':
    case ']':
    case '(':
    case ')':
    case '?':
    case ':':
    case ';':
    case ',':
    case '.':
    case '{':
    case '}':
      return true; 
      break;
    default:
      return false;
      break;
  }
  return false;
}

bool
IsWhitespace(char Character) {
  switch (Character) {
    case ' ':  // space
    case '\0': // null
    case '\r': // cariage return
    case '\n': // newline
    case '\t': // tab
    case '\f': // feed
    case '\v': // vertical tab
      return true;
      break;
    default:
      return false;
      break;
  }
  return false;
}

bool
IsConstant(const char *Start, u32 Length) {
  if (Length > 2 && Start[0] == '0' && Start[1] == 'x') {
    for (u32 Index = 2;
         Index < Length;
         ++Index) {
      if (!IsDigit(Start[Index]) &&
          !(Start[Index] >= 'a' && Start[Index] <= 'f') &&
          !(Start[Index] >= 'A' && Start[Index] <= 'F'))
        return false;
    }
    return true;
  } else if (Length == 3 && *Start == '\'') {
    if (Start[2] == '\'')
      return true;
    else
      return false;
  } else {
    bool WasDot = false;
    for (u32 Index = 0;
         Index < Length;
         ++Index) {
      if (Start[Index] == '.') {
        if (!WasDot) {
          WasDot = true;
          continue;
        } else {
          return false;
        }
      }
      
      if (Start[Index] == 'f' && Index == Length - 1 && Length > 1)
        return true;

      if (!IsDigit(Start[Index]))
        return false;
    }
    return true;
  }
  return false;
}

u32
GetMaxLinesOnPanel(u32 Height, u32 LineHeight) {
  // TODO(Brajan): here, yay
  u32 MarginTop = BCoderState.LineHeight + 4;
  u32 StartY = MarginTop;
  u32 SpaceY = Height - MarginTop;
  u32 MaxLines = (u32)(SpaceY / LineHeight);
  if (SpaceY % LineHeight != 0)
    MaxLines -= 1;
  return MaxLines;
}

void
PanelCursorMoved(panel *Panel) {
  u32 CursorLinePosition = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
  u32 NumVisibleLines = GetMaxLinesOnPanel(Panel->DrawAreaHeight, BCoderState.LineHeight);
  if (CursorLinePosition > Panel->ViewLine + NumVisibleLines) {
    Panel->ViewLine = CursorLinePosition - NumVisibleLines;
  } else if (CursorLinePosition < Panel->ViewLine) {
    Panel->ViewLine = CursorLinePosition;
  }
}

void
PanelViewChanged(panel *Panel) {
  u32 CursorLinePosition = GetLineNumberByPosition(Panel->Buffer, Panel->CursorPosition);
  u32 NumVisibleLines = GetMaxLinesOnPanel(Panel->DrawAreaHeight, BCoderState.LineHeight);
  if (CursorLinePosition > Panel->ViewLine + NumVisibleLines) {
    Panel->CursorPosition = GetPositionWhereLineStarts(Panel->Buffer, Panel->ViewLine + NumVisibleLines);
  } else if (CursorLinePosition < Panel->ViewLine) {
    Panel->CursorPosition = GetPositionWhereLineStarts(Panel->Buffer, Panel->ViewLine);
  }
}

void
PanelShowCommandPrompt(panel *Panel, const char *StartCmdBuffer) {
  Assert(Panel != 0);
  
  Panel->ShowCommandPrompt = true;
  Panel->CommandCursorPosition = 0;
  if (Panel->CommandBuffer.Memory == 0)
    Panel->CommandBuffer = Platform.CreateEmptyBuffer(Kilobytes(1));
  // TODO(Brajan): change that because this can be reason of some memory problems in future!!!
  ZeroMemory(Panel->CommandBuffer.Memory, Kilobytes(1));

  if (StartCmdBuffer) {
    Panel->CommandCursorPosition += InsertString(&Panel->CommandBuffer, Panel->CommandCursorPosition, (char *)StartCmdBuffer, StringLength((char *)StartCmdBuffer));
  }
}

bool
PanelEditFile(panel *Panel, const char *Filename) {
  Assert(Panel != 0);
  if (Panel->Buffer) {
    buffer *Buffer = Panel->Buffer;
    Panel->Buffer = 0;
    CloseBuffer(Buffer);
  }
  Panel->CursorPosition = 0;
  Panel->Selecting = false;
  Panel->Buffer = GetBuffer(Filename);
  return true;
}

bool
PanelJumpToLine(panel *Panel, u32 Line) {
  Assert(Panel != 0);
  if (Line > GetLinesNumber(Panel->Buffer))
    return false;

  Panel->CursorPosition = GetPositionWhereLineStarts(Panel->Buffer, Line);
  PanelCursorMoved(Panel);
  return true;
}

u32
GetNumberOfOpenPanels() {
  u32 Num = 0;
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (BCoderState.Panels[Index].Active)
      ++Num;
  }
  return Num;
}

void
PanelUpdate(panel *Panel) {
  Platform.Redraw();
}

void
ParseAndExecuteOnPanel(panel *Panel) {
  Assert(Panel != 0);
  Assert(Panel->ShowCommandPrompt);

  // TODO(Brajan): Regex expressions
}

void
PushCharacterToBuffer(buffer *Buffer, char Character) {
  Assert(Buffer != 0);
  char *StringBuffer = (char *)Buffer->Memory;
  StringBuffer[Buffer->Length++] = Character;
  Buffer->Flags &= ~(BufferFlag_Saved);
}

void
PopCharacterFromBuffer(buffer *Buffer) {
  Assert(Buffer != 0);
  if (Buffer->Length <= 0)
    return;
  char *StringBuffer = (char *)Buffer->Memory;
  StringBuffer[--Buffer->Length] = '\0';
  Buffer->Flags &= ~(BufferFlag_Saved);
}

char
GetCharacterAtPosition(buffer *Buffer, u32 Position) {
  Assert(Buffer != 0);
  Assert(Position < Buffer->Length);
  return ((char *)Buffer->Memory)[Position];
}

void
StringMove(char *Destination, char *Source, u32 Count) {
  char *Temp;
  char *String;
  if (Destination <= Source) {
    Temp = Destination;
    String = Source;
    while (Count--)
      *Temp++ = *String++;
  } else {
    Temp = Destination;
    Temp += Count;
    String = Source;
    String += Count;
    while (Count--)
      *--Temp = *--String;
  }
}

u32
InsertString(buffer *Buffer, u32 Position, char *String, u32 Length) {
  Assert(Buffer != 0);
  Assert(String != 0);
  
  if (Buffer->Flags & BufferFlag_NotEditable) {
    return 0;
  }
  
  StringMove((char *)Buffer->Memory + Position + Length, (char *)Buffer->Memory + Position, Buffer->Length - Position);
  for (u32 Index = 0;
       Index < Length;
       ++Index) {
    *((char *)Buffer->Memory + Position + Index) = String[Index];
  }
  Buffer->Length += Length;
  Buffer->Flags &= ~(BufferFlag_Saved);
  return Length;
}

u32
RemoveCharacters(buffer *Buffer, u32 Position, u32 Num) {
  Assert(Buffer != 0);
  
  if (Buffer->Flags & BufferFlag_NotEditable) {
    return 0;
  }

  StringMove((char *)Buffer->Memory + Position, (char *)Buffer->Memory + Position + Num, Buffer->Length - Position);
  for (u32 Index = 0;
       Index < Num;
       ++Index) {
    *((char *)Buffer->Memory + Buffer->Length + Index) = '\0';
  }
  Buffer->Length -= Num;
  Buffer->Flags &= ~(BufferFlag_Saved);
  return Num;
}

void
InsertCharacter(buffer *Buffer, u32 Position, char Character) {
  Assert(Buffer != 0);
  
  if (Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  if (Position == Buffer->Length) {
    PushCharacterToBuffer(Buffer, Character);
    return;
  }

  StringMove((char *)Buffer->Memory + Position + 1, (char *)Buffer->Memory + Position, Buffer->Length - Position); 
  ((char *)Buffer->Memory)[Position] = Character;
  ++Buffer->Length;
  Buffer->Flags &= ~(BufferFlag_Saved);
}

void
RemoveCharacter(buffer *Buffer, u32 Position) {
  Assert(Buffer != 0);
  
  if (Buffer->Flags & BufferFlag_NotEditable) {
    return;
  }

  if (Position == 0)
    return;
  
  StringMove((char *)Buffer->Memory + Position - 1, (char *)Buffer->Memory + Position, Buffer->Length - Position);
  --Buffer->Length;
  *((char *)Buffer->Memory + Buffer->Length) = '\0';
  Buffer->Flags &= ~(BufferFlag_Saved);
}

u32
GetPositionWhereLineStarts(buffer *Buffer, u32 Line) {
  Assert(Buffer != 0);
  if (Line == 0)
    return 0;
  u32 LineCounter = 0;
  u32 Position = 0;
  char *String = (char *)Buffer->Memory;
  while (String[Position] != '\0') {
    if (String[Position] == '\n') {
      if (++LineCounter == Line)
        return Position + 1;
    }

    ++Position;
  }

  return 0;
}

u32
CalculateCursorPositionInLine(buffer *Buffer, u32 Position) {
  Assert(Buffer != 0);
  u32 Line = GetLineNumberByPosition(Buffer, Position);
  return Position - GetPositionWhereLineStarts(Buffer, Line);
}

u32
GetLineNumberByPosition(buffer *Buffer, u32 Position) {
  Assert(Buffer != 0);

  u32 Line = 0;
  char *StringBuffer = (char *)Buffer->Memory;
  for (u32 Index = 0;
       Index < Buffer->Length;
       ++Index) {
    if (Index == Position) {
      return Line;
    }
    if (StringBuffer[Index] == '\n') {
      ++Line;
    }
  }
  return 0;
}

u32
GetLineLength(buffer *Buffer, u32 Line) {
  Assert(Buffer != 0);
  u32 LineStartPosition = GetPositionWhereLineStarts(Buffer, Line);
  u32 LineEndPosition = LineStartPosition;
  char *StringBuffer = (char *)Buffer->Memory;
  while (StringBuffer[LineEndPosition] != '\r' && StringBuffer[LineEndPosition] != '\n' && StringBuffer[LineEndPosition] != '\0')
    ++LineEndPosition;
  return LineEndPosition - LineStartPosition; 
}

u32
GetLinesNumber(buffer *Buffer) {
  Assert(Buffer != 0);

  u32 Line = 0;
  char *StringBuffer = (char *)Buffer->Memory;
  for (u32 Index = 0;
       Index < Buffer->Length;
       ++Index) {
    if (StringBuffer[Index] == '\n') {
      ++Line;
    }
  }
  return Line;
}


void
AddInsertHistory(buffer *Buffer, u32 Position, u32 Length) {
  Assert(Buffer != 0);
  Assert(Position + Length < Buffer->Length);

  if (Buffer->HistoryStackPosition >= MaxHistoryStackSize) {
    return;
  }

  history_element *Element = &Buffer->HistoryStack[Buffer->HistoryStackPosition++];
  Element->Type = History_Insert;
  Element->Position = Position;
  Element->Length = Length;
  Element->Data = 0;
}

