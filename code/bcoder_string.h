// NOTE(Brajan): We operate on wchar_t here
#ifndef BCODER_STRING_H_

s32 StringCompare(const char *A, const char *B);
s32 StringCompareLength(const char *A, const char *B, u32 Length);
char *StringCopy(char *Destination, const char *Source);
u32 StringLength(char *String);

bool IsDigit(char Character);
bool IsLetter(char Character);
s32 StringToInt(const char *String);

#define BCODER_STRING_H_
#endif

