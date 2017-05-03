%% TRAXCLIENT - MEX implementation of TraX client
%
% This function provides client side TraX protocol bindings.
% It can be used to run external tracker processes and communicate
% with them interactivelly.
%
% [data] = TRAXCLIENT(command, callback, varargs)
%
% command - a command line string to execute
% callback - callback function called for every protocol event
%
% Other arguments:
%  * directory - working directory of the process
%  * timeout - timeout in seconds
%  * environment - additional environmental variables
%  * connection - connection type
%  * data - custom data passed to callback
%
% For more information consult the online TraX reference
% library documentation.
