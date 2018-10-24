function blkStruct = slblocks

%   Copyright 1990-2004 The MathWorks, Inc.
%   $Revision: 9142 $

%
% Name of the subsystem which will show up in the Simulink Blocksets
% and Toolboxes subsystem.
%
blkStruct.Name = ['Uei PowerDAQ' sprintf('\n') 'Library'];

%
% The function that will be called when the user double-clicks on
% this icon.
%
blkStruct.OpenFcn = 'uei_lib;';%.mdl file

%
% The argument to be set as the Mask Display for the subsystem.  You
% may comment this line out if no specific mask is desired.
% Example:  blkStruct.MaskDisplay = 'plot([0:2*pi],sin([0:2*pi]));';
% No display for Simulink Extras.
%
blkStruct.MaskDisplay = 'disp(''UEI'')';

%
% Define the Browser structure array, the first element contains the
% information for the Simulink block library and the second for the
% Simulink Extras block library.
%
Browser(1).Library = 'uei_lib';
Browser(1).Name    = 'UEI PowerDAQ Library';
Browser(1).IsFlat  = 1;% Is this library "flat" (i.e. no subsystems)?

blkStruct.Browser = Browser

% End of slblocks


