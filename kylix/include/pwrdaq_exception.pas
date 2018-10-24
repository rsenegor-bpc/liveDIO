unit pwrdaq_exception;

interface

uses
   SysUtils;

type
   EPwrDAQ = class(Exception)
   public
    errorCode : Integer;
    constructor Create(source: String; error : Integer);
  end; // EPwrDAQ

implementation

constructor EPwrDAQ.Create(source: String; error : Integer);
var
  Msg : string;
begin
  Msg := 'Error ' + IntToStr(error) + ' in ' + source;
  errorCode := error;
  inherited Create(Msg);
end;

end.
 