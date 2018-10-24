function makeInfo = rtwmakecfg()
% RTWMAKECFG Add include and source directories to RTW make files.
% makeInfo = RTWMAKECFG returns a structured array containing following fields:

% makeInfo.includePath - cell array containing additional include
% directories. Those directories will be
% expanded into include instructions of rtw
% generated make files.

% makeInfo.sourcePath - cell array containing additional source
% directories. Those directories will be
% expanded into rules of rtw generated make
% files.

% makeInfo.library - structure containing additional runtime library
% names and module objects. This information
% will be expanded into rules of rtw generated make
% files.
% ... .library(1).Name - name of runtime library
% ... .library(1).Modules - cell array containing source file
% names for the runtime library
% ... .library(2).Name
% ... .library(2).Modules
% ...

% This RTWMAKECFG file must be located in the same directory as the related
% S-function MEX-DLL(s). If one or more S-functions of the directory are referenced
% by a model Real-Time Workshop will evaluate RTWMAKECFG to obtain the additional
% include and source directories.

% To examine more RTWMAKECFG files in your installation issue at the MATLAB prompt:
% >> which RTWMAKECFG -all

% Issue a message.
seperatorLine = char(ones(1,70) * '~');
fprintf('\n');
fprintf('%s\n', seperatorLine);
fprintf(' %s\n', which(mfilename));
fprintf(' Adding source and include directories to make process.\n')

% Setting up the return structure with
% - source directories:
% C:\Projects\SFunctions\Sources
makeInfo.sourcePath = { ...
      strcat(matlabroot, '/toolbox/uei') ...
   };

% - include directories
% C:\Projects\SFunctions\Headers
makeInfo.includePath = { ...
      strcat(getenv('PDROOT'), '/include') ...
   };

%makeInfo.precompile = 1;
%makeInfo.library(1).Name     = 'pwrdaq32';
%makeInfo.library(1).Location = strcat(getenv('PDROOT'), '\lib\pwrdaq32.lib');
%makeInfo.library(1).Modules  = {'srcfile1' 'srcfile2' 'srcfile3' };

% Display contents.
fprintf(' - additional source directories:\n');
fprintf(' %s\n', makeInfo.sourcePath{:});
fprintf(' - additional include directories:\n');
fprintf(' %s\n', makeInfo.includePath{:});
fprintf('%s\n', seperatorLine);

% [EOF] rtwmakecfg.m