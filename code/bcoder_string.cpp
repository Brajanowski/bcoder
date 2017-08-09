#include "bcoder_string.h"

s32
StringCompare(const char *A, const char *B) {
  while (*A && (*A++ == *B++));
  return *A - *B;
}

s32
StringCompareLength(const char *A, const char *B, u32 Length) {
  while (Length--)
    if (*A++ != *B++)
      return *(unsigned char *)(A - 1) - *(unsigned char *)(B - 1);
  return 0;
}

char *
StringCopy(char *Destination, const char *Source) {
  char *Result = Destination;
  while (*Destination++ = *Source++);
  return Destination;
}

u32
StringLength(char *String) {
  char *Begin = String;
  while (*(++String));
  return (u32)(String - Begin);
}

bool
IsLetter(char Character) {
  if ((Character >= 'A' && Character <= 'Z') || (Character >= 'a' && Character <= 'z'))
    return true;
  return false;
}

bool
IsDigit(char Character) {
  if (Character >= '0' && Character <= '9')
    return true;
  return false;
}

s32
StringToInt(const char *String) {
  if (String == 0)
    return 0;

  s32 Value = 0;
  s32 Sign = 1;
  if (*String == '+') {
    ++String;
  } else if (*String == '-') {
    Sign = -1;
    ++String;
  }

  while (IsDigit(*String)) {
    Value *= 10;
    Value += (s32)(*String - '0');
    ++String;
  }
  return Sign * Value;
}
