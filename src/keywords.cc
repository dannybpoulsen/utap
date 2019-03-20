/* C++ code produced by gperf version 3.0.1 */
/* Command-line: gperf -C -E -t -L C++ -c -Z Keywords keywords.gperf  */
/* Computed positions: -k'1-2' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "keywords.gperf"
struct Keyword { char *name; int token; int32_t syntax; };
/* maximum key range = 53, duplicates = 0 */

class Keywords
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct Keyword *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
Keywords::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 10, 15, 35,
       0, 20,  5,  5,  5, 10, 55, 55, 30,  0,
      20,  0,  0,  0,  5,  0,  0, 35, 55, 15,
      55, 15, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]];
}

const struct Keyword *
Keywords::in_word_set (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 38,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 13,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 54
    };

  static const struct Keyword wordlist[] =
    {
      {""}, {""},
#line 26 "keywords.gperf"
      {"do", 			T_DO,		SYNTAX_NEW},
      {""}, {""},
#line 9 "keywords.gperf"
      {"state",			T_STATE,	SYNTAX_OLD | SYNTAX_NEW},
#line 36 "keywords.gperf"
      {"struct",			T_STRUCT,	SYNTAX_NEW},
#line 21 "keywords.gperf"
      {"or",			T_KW_OR,	SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 24 "keywords.gperf"
      {"for",			T_FOR,		SYNTAX_NEW},
#line 18 "keywords.gperf"
      {"true",			T_TRUE,		SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 11 "keywords.gperf"
      {"trans",			T_TRANS,	SYNTAX_OLD | SYNTAX_NEW},
#line 41 "keywords.gperf"
      {"forall", 		T_FORALL,	SYNTAX_NEW | SYNTAX_PROPERTY},
#line 8 "keywords.gperf"
      {"process",		T_PROCESS,	SYNTAX_OLD | SYNTAX_NEW},
#line 40 "keywords.gperf"
      {"progress",		T_PROGRESS,	SYNTAX_NEW},
      {""},
#line 23 "keywords.gperf"
      {"imply",			T_KW_IMPLY,	SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 7 "keywords.gperf"
      {"assign",			T_ASSIGN,	SYNTAX_OLD | SYNTAX_NEW},
#line 30 "keywords.gperf"
      {"if", 			T_IF,		SYNTAX_NEW},
#line 45 "keywords.gperf"
      {"proc_priority",		T_PROC_PRIORITY, SYNTAX_OLD | SYNTAX_NEW},
#line 6 "keywords.gperf"
      {"sync",			T_SYNC,		SYNTAX_OLD | SYNTAX_NEW},
#line 19 "keywords.gperf"
      {"false",			T_FALSE,	SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 17 "keywords.gperf"
      {"system",			T_SYSTEM,	SYNTAX_OLD | SYNTAX_NEW},
#line 35 "keywords.gperf"
      {"typedef",		T_TYPEDEF,	SYNTAX_NEW},
#line 22 "keywords.gperf"
      {"not",			T_KW_NOT,	SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 37 "keywords.gperf"
      {"meta",			T_META,		SYNTAX_NEW},
#line 25 "keywords.gperf"
      {"while",			T_WHILE,	SYNTAX_NEW},
#line 4 "keywords.gperf"
      {"select",			T_SELECT,	SYNTAX_NEW},
#line 39 "keywords.gperf"
      {"after_update",		T_AFTER,	SYNTAX_NEW},
#line 42 "keywords.gperf"
      {"deadlock",		T_DEADLOCK,	SYNTAX_PROPERTY},
#line 16 "keywords.gperf"
      {"broadcast",       	T_BROADCAST,	SYNTAX_OLD | SYNTAX_NEW},
      {""},
#line 34 "keywords.gperf"
      {"return", 		T_RETURN,	SYNTAX_NEW},
#line 14 "keywords.gperf"
      {"winning",		T_WINNING,	SYNTAX_NEW},
#line 20 "keywords.gperf"
      {"and",			T_KW_AND,	SYNTAX_OLD | SYNTAX_NEW | SYNTAX_PROPERTY},
#line 10 "keywords.gperf"
      {"init",			T_INIT,		SYNTAX_OLD | SYNTAX_NEW},
      {""},
#line 15 "keywords.gperf"
      {"losing",			T_LOSING,	SYNTAX_NEW},
      {""}, {""},
#line 43 "keywords.gperf"
      {"quit",			T_QUIT,		SYNTAX_PROPERTY},
#line 3 "keywords.gperf"
      {"const",			T_CONST,	SYNTAX_OLD | SYNTAX_NEW},
#line 13 "keywords.gperf"
      {"commit",			T_COMMIT,	SYNTAX_OLD | SYNTAX_NEW},
      {""}, {""}, {""},
#line 5 "keywords.gperf"
      {"guard",			T_GUARD,	SYNTAX_OLD | SYNTAX_NEW},
#line 12 "keywords.gperf"
      {"urgent",			T_URGENT,	SYNTAX_OLD | SYNTAX_NEW},
      {""},
#line 38 "keywords.gperf"
      {"before_update",		T_BEFORE,	SYNTAX_NEW},
      {""}, {""}, {""}, {""},
#line 44 "keywords.gperf"
      {"chan_priority",		T_CHAN_PRIORITY, SYNTAX_OLD | SYNTAX_NEW},
#line 31 "keywords.gperf"
      {"else", 			T_ELSE,		SYNTAX_NEW}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
#line 46 "keywords.gperf"


bool isKeyword(const char *id, uint32_t syntax)
{
    const Keyword *keyword = Keywords::in_word_set(id, strlen(id));
    return (keyword != NULL && (keyword->syntax & syntax));
}
