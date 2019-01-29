function simple_client(command, multichannel)

    data.num = 10;

    environment = struct();

    if (multichannel)
        environment.TRAX_TEST_USE_DEPTH = '1';
        environment.TRAX_TEST_USE_IR = '1';
    end;

    traxclient(command, @callback, 'Data', data, 'Environment', environment);

end

function [image, region, properties, data] = callback(state, data)

    image = {};
    region = [100, 100, 200, 200];
    properties = struct();

    if data.num < 1
        return;
    end

    if ~isempty(state.region)
        region = [];
    end

    properties.wait = 1;

    data.num = data.num - 1;

    if any(ismember(state.channels, 'color'))
        image{end+1} = uint8(rand(200, 200, 3) * 255);
    end;

    if any(ismember(state.channels, 'depth'))
        image{end+1} = uint16(rand(200, 200, 1) * 255);
    end;

    if any(ismember(state.channels, 'ir'))
        image{end+1} = uint16(rand(200, 200, 1) * 255);
    end;

end
