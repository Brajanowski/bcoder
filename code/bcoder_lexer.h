#ifndef BCODER_LEXER_H_

#define MaxRules 64
#define MaxTokenValueLength 256
#define MaxRegionEdgeLength 24
#define MaxKeywordLength 32

enum token_type {
  Token_Unknown,
  Token_EOF,
  Token_Whitespace,
  Token_Keyword,
  Token_Constant,
  Token_String,
  Token_Comment,
  Token_Operator,
  Token_Preprocesor,
  Token_PlainText
};

enum rule_type {
  Rule_Region,
};

enum region_type {
  Region_String,
  Region_Comment,
  Region_Preprocesor
};

enum region_edge_type {
  RegionEdge_SingleCharacter,
  RegionEdge_String,
  // NOTE(Brajan): only for region ending
  RegionEdge_EOF
};

struct rule {
  bool Enabled;
  union {
    struct {
      int MarkAs;
      bool WrapToEndOfLine;

      int StartEdgeType;
      union {
        char StartSingleCharacter;
        char StartString[MaxKeywordLength]; 
      };

      int EndEdgeType;
      union {
        char EndSingleCharacter;
        char EndString[MaxKeywordLength];
      };
    } Region;
    
    struct {
      char Value[MaxKeywordLength];
    } Keyword;
  };
};

struct rules_context {
  rule RegionRules[MaxRules];
  rule KeywordRules[MaxRules];
};

struct token {
  int Type;
  int Level;
  char Value[MaxTokenValueLength];
  int Length;
};

struct lexer_context {
  token *Tokens;
  int TokensNumber;
  int TokensCapacity;
};

void LexerInit(lexer_context *Context);
void LexerShutdown(lexer_context *Context);
void Tokenize(lexer_context *Context, char *String, int Length);

#define BCODER_LEXER_H_
#endif
