function [] = badtracker()

traxserver('setup', 'polygon', {'path', 'memory', 'buffer'});

memory = [0 0 0 0];

when = 10;
cause = 'abort';

while 1

    [image, region, props] = traxserver('wait', 'PropStruct', true);

    if isempty(image)
		break;
	end;

	if ~isempty(region)
        memory = region;

		if ~isempty(props)
			if isfield(props, 'break_type')
				cause = props.break_type;
			end;
			if isfield(props, 'break_time')
				when = str2double(props.break_time);
			end;
		end;

    end

	if when <= 0
		badmex(cause);
		break;
	end;

	when = when - 1;

	traxserver('status', memory);
end

traxserver('quit');
