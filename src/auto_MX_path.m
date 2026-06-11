function out = auto_MX_path()
    temp_MicroMouse_MX_path = string(fullfile(pwd, 'MicroMouseProgramming_Code', 'MicroMouseProgramming.ioc'))
    
    if exist(temp_MicroMouse_MX_path) == 2
        disp("MicroMouse : STM32CubeMX IOC file found!")
        out = temp_MicroMouse_MX_path;
        MX_IOC_LOCATION = temp_MicroMouse_MX_path;
    else
        error("MicroMouse : You are working in the wrong directory / you do not have all the neccesary files")
    end
end