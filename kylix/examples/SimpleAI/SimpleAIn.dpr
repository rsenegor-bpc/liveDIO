program SimpleAIn;

uses
  QForms,
  SimpleAI in 'SimpleAI.pas' {Form1},
  pwrdaq_struct in '../../include/pwrdaq_struct.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
