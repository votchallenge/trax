function [] = trax_status(obj, region)

switch obj.format_region
case 'rectangle'
    fprintf('@@TRAX:status %.2f,%.2f,%.2f,%.2f\n', region(1), region(2), region(3), region(4));
otherwise
    error('Unsupported region format');
end;



