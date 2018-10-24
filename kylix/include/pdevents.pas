unit pdevents; // from C:\PowerDAQ\pwrdaq32\pdevents.h
interface
uses
  windows;

{$ALIGN OFF}

(*
type
  PD_UCT_EVENT = record
    Uct0 : UCHAR;
    Uct1 : UCHAR;
    Uct2 : UCHAR;
  end;
var
  _PD_UCT_EVENT : PD_UCT_EVENT;
*)

type
  DWORD = Cardinal;
  PPD_EVENTS = ^PD_EVENTS;
  PD_EVENTS = record
    BoardStatus : ULONG;
    AinStatus : ULONG;
    AoutStatus : ULONG;
    DinStatus : ULONG;
    UctStatus : ULONG;
  end;
var
  _PD_EVENTS : PD_EVENTS;

{$ALIGN ON}
implementation
begin
end.

