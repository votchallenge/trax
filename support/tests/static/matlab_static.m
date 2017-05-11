function [] = matlab_static()

cleanup = onCleanup(@() exit() );

traxserver('setup', 'polygon', {'path', 'memory', 'buffer'});

memory = [0 0 0 0];

timeout = 0;

while 1

    [image, region, properties] = traxserver('wait', 'PropStruct', true);

    if isempty(image)
		break;
	end;

	if ~isempty(region)
		if isfield(properties, 'time_wait')
			timeout = str2double(properties.time_wait);
		end;
        memory = region;
    end;

    if timeout > 0
    	pause(timeout);
    end;

	traxserver('status', memory);
end;

traxserver('quit');
