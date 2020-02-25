%% TRAXSERVER - MEX bindings for TraX protocol
%
% This function provides server side TraX protocol bindings.
% It can be used in various modes to set-up tracking session,
% send and receive data and terminate the communication.
%
% Protocol setup:
% [response] = traxserver('setup', region_formats, image_formats, ...)
%
% Waiting for request:
% [image, region, parameters] = traxserver('wait')
%
% Reporting state:
% [response] = traxserver('state', region, parameters)
%
% Terminating the session:
% [response] = traxserver('quit', reason)
%
% For more information consult the online TraX reference
% library documentation.
