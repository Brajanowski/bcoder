#include "bcoder_lexer.h"
#include "bcoder.h"

#define StartTokensCapacity 1024

internal void RemoveAllTokens(lexer_context *Context);
internal void PushToken(lexer_context *Context, token Token);
internal bool IsWhitespaceLexer(char Character);
internal int RegionTypeToTokenType(int RegionType);
internal bool IsDelimiter(char Character);
internal bool IsDigit(char Character);

global_variable rules_context _RulesContext;

internal void
SetKeyword(int Index, char *Value) {
  _RulesContext.KeywordRules[Index].Enabled = true;
  StringCopy(_RulesContext.KeywordRules[Index].Keyword.Value, Value);
}

void
LexerInit(lexer_context *Context) {
  Assert(Context != 0);
  Context->Tokens = (token *)Platform.AllocateMemory(sizeof(token) * StartTokensCapacity);
  Context->TokensNumber = 0;
  Context->TokensCapacity = StartTokensCapacity;

  // NOTE(Brajan): It's temporary solution for single language only.
  // I'm gonna want to move this to some rules database or something like that.
  _RulesContext.RegionRules[0].Enabled = true;
  _RulesContext.RegionRules[0].Region.MarkAs = Region_String;
  _RulesContext.RegionRules[0].Region.WrapToEndOfLine = true;
  _RulesContext.RegionRules[0].Region.StartEdgeType = RegionEdge_SingleCharacter;
  _RulesContext.RegionRules[0].Region.StartSingleCharacter = '"';
  _RulesContext.RegionRules[0].Region.EndEdgeType = RegionEdge_SingleCharacter;
  _RulesContext.RegionRules[0].Region.EndSingleCharacter = '"';

  // single line comment
  _RulesContext.RegionRules[1].Enabled = true;
  _RulesContext.RegionRules[1].Region.MarkAs = Region_Comment;
  _RulesContext.RegionRules[1].Region.WrapToEndOfLine = true;
  _RulesContext.RegionRules[1].Region.StartEdgeType = RegionEdge_String;
  _RulesContext.RegionRules[1].Region.StartString[0] = '/';
  _RulesContext.RegionRules[1].Region.StartString[1] = '/';
  _RulesContext.RegionRules[1].Region.StartString[3] = '\0';
  _RulesContext.RegionRules[1].Region.EndEdgeType = RegionEdge_EOF;

  // preprocesor
  _RulesContext.RegionRules[2].Enabled = true;
  _RulesContext.RegionRules[2].Region.MarkAs = Region_Preprocesor;
  _RulesContext.RegionRules[2].Region.WrapToEndOfLine = true;
  _RulesContext.RegionRules[2].Region.StartEdgeType = RegionEdge_SingleCharacter;
  _RulesContext.RegionRules[2].Region.StartSingleCharacter = '#';
  _RulesContext.RegionRules[2].Region.EndEdgeType = RegionEdge_EOF;

  int Index = 0;
  SetKeyword(Index++, "struct");
  SetKeyword(Index++, "class");
  SetKeyword(Index++, "int");
  SetKeyword(Index++, "float");
  SetKeyword(Index++, "double");
  SetKeyword(Index++, "break");
  SetKeyword(Index++, "continue");
  SetKeyword(Index++, "unsigned");
  SetKeyword(Index++, "char");
}

void
LexerShutdown(lexer_context *Context) {
  Assert(Context != 0);
  Assert(Context->Tokens != 0);

  Platform.FreeMemory((void *)Context->Tokens);
}

void
Tokenize(lexer_context *Context, char *String, int Length) {
  RemoveAllTokens(Context);

  rules_context *RulesContext = &_RulesContext;

  for (int Index = 0;
       Index < Length;
       ++Index) {

    if (String[Index] == '\r') {
      ; // ignore
    } else if (String[Index] == '\n') {
      token Token = {};
      Token.Type = Token_EOF;
      PushToken(Context, Token);
    } else if (IsWhitespaceLexer(String[Index])) {
      int Num = 1;

      while (IsWhitespaceLexer(String[Index++] && Index < Length))
        ++Num;
      Index--;

      token Token = {};
      Token.Type = Token_Whitespace;
      Token.Length = Num;
      PushToken(Context, Token);
    } else {
      // testing for region
      bool IsMatchingRegion = false;
      int Start = 0;
      int End = 0;
      int StartEdgeLength = 1;
      int RegionType = 0;

      for (int RuleIndex = 0;
           RuleIndex < MaxRules;
           ++RuleIndex) {
        rule *Rule = &RulesContext->RegionRules[RuleIndex];
        if (!Rule->Enabled)
          continue;
        
        bool IsStartMatching = false;

        if (Rule->Region.StartEdgeType == RegionEdge_SingleCharacter) {
          if (Rule->Region.StartSingleCharacter == String[Index]) {
            IsStartMatching = true;
          }
        } else if (Rule->Region.StartEdgeType == RegionEdge_String) {
          char *Str1 = Rule->Region.StartString;
          char *Str2 = String + Index;
          bool IsStringMatching = true;
          while (*Str1) {
            if (*Str1 != *Str2) {
              IsStringMatching = false;
            }
            ++Str1;
            ++Str2;
          }
          
          if (IsStringMatching) {
            IsStartMatching = true;
            StartEdgeLength = StringLength(Rule->Region.StartString);
          }
        }

        if (!IsStartMatching)
          continue;

        if (Rule->Region.EndEdgeType == RegionEdge_SingleCharacter) {
          if (Rule->Region.WrapToEndOfLine) {
            int Position = Index + StartEdgeLength;
            bool Found = false;
            while (String[Position] != '\n' && Position < Length) {
              if (Rule->Region.EndSingleCharacter == String[Position]) {
                Found = true;
                break;
              }
              ++Position;
            }

            if (Found) {
              IsMatchingRegion = true;
              Start = Index;
              End = Position;
            }
          } else {

          }
        } else if (Rule->Region.EndEdgeType == RegionEdge_EOF) {
          int Position = Index + StartEdgeLength;
          while (String[Position] != '\n' && Position < Length) {
            ++Position;
          }

          IsMatchingRegion = true;
          Start = Index;
          End = Position - 1;
        }

        RegionType = Rule->Region.MarkAs;
      }

      if (IsMatchingRegion) {
        token Token = {};
        Token.Type = RegionTypeToTokenType(RegionType);

        MemoryCopy((void *)(String + Start), (void *)Token.Value, End - Start + 1);
        Token.Length = End - Start + 1;
        Index = End;
        PushToken(Context, Token);
        // we are done
        continue;
      }

      // testing for delimiters
      if (IsDelimiter(String[Index])) {
        token Token = {};
        Token.Type = Token_Operator;
        Token.Length = 1;
        Token.Value[0] = String[Index];
        PushToken(Context, Token);
        // done
        continue;
      }

      if (IsDigit(String[Index]) || IsLetter(String[Index]) || String[Index] == '_') {
        bool IsKeyword = false;




        if (IsKeyword) {
          continue;
        }
      }

      // just a plain text
      token Token = {};
      Token.Type = Token_PlainText;
      Token.Value[0] = String[Index];
      Token.Length = 1;
      PushToken(Context, Token);
    }
  }
}

internal void
RemoveAllTokens(lexer_context *Context) {
  Assert(Context != 0);
  Assert(Context->Tokens != 0);

  Context->TokensNumber = 0;
}

internal void
PushToken(lexer_context *Context, token Token) {
  Assert(Context != 0);
  Assert(Context->Tokens != 0);

  if (Context->TokensNumber >= Context->TokensCapacity) {
    int NewCapacity = Context->TokensCapacity * 2;
    void *NewPointer = Platform.AllocateMemory(sizeof(token) * NewCapacity);
    Assert(NewPointer != 0);

    MemoryCopy((void *)Context->Tokens, NewPointer, sizeof(token) * Context->TokensNumber);
    Platform.FreeMemory((void *)Context->Tokens);

    Context->Tokens = (token *)NewPointer;
    Context->TokensCapacity = NewCapacity;
  }

  Context->Tokens[Context->TokensNumber++] = Token;
}

internal bool
IsWhitespaceLexer(char Character) {
  if (Character == ' ' ||
      Character == '\t')
    return true;
  return false;
}

// TODO(Brajan): Add more region types
internal int
RegionTypeToTokenType(int RegionType) {
  if (RegionType == Region_String)
    return Token_String;
  else if (RegionType == Region_Comment)
    return Token_Comment;
  else if (RegionType == Region_Preprocesor)
    return Token_Preprocesor;
  return -1;
}

internal bool
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

internal bool
IsDigit(char Character) {
  if (Character >= '0' && Character <= '9')
    return true;
  return false;
}

