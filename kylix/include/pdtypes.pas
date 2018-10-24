unit pdtypes;

interface

{$IFDEF VER130}
uses Windows;
{$ENDIF}

type
   I32=Integer;
   U32=Cardinal;
   I16=SmallInt;
   U16=Word;
   I8=ShortInt;
   U8=Byte;

   DWORD=Integer;
   PPWORD=^PWORD;
   BOOL=LongBool;
   PBOOL=^BOOL;
   ULONG=Cardinal;
   PDWORD=^DWORD;
   PULONG=^ULONG;
   LPSTR=PCHAR;
   SHORT=SmallInt;
   PHANDLE=^THANDLE;

implementation

end.
