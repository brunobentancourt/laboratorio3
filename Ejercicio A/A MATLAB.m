puertoCOM = 'COM4'; 
baudRate = 9600;

if exist('s', 'var') && isvalid(s)
    clear s;
end

disp('Conectando al microcontrolador en COM4...');
s = serialport(puertoCOM, baudRate);
configureTerminator(s, "CR/LF");
flush(s);

disp('Conectado exitosamente. Generando gráfica en tiempo real...');

figure('Name', 'Sistema de Control de Temperatura - ATmega328P', 'NumberTitle', 'off');
h_temp = animatedline('Color', 'b', 'LineWidth', 2, 'Marker', 'o', 'DisplayName', 'Temp. Actual (°C)');
h_pm   = animatedline('Color', 'r', 'LineStyle', '--', 'LineWidth', 1.5, 'DisplayName', 'Punto Medio (°C)');

xlabel('Tiempo (Muestras cada 5s)');
ylabel('Temperatura (°C)');
title('Monitoreo Térmico en Tiempo Real');
grid on;
legend('Location', 'northwest');
ylim([0 50]);

muestra = 0;


while ishandle(1)
    try
        linea = readline(s);
        
        
        if contains(linea, '---') || contains(linea, 'ERROR') || contains(linea, 'actualizado')
            disp(linea);
            continue;
        end
        
        datos = strsplit(strtrim(linea), ',');
        
        if length(datos) == 3
            temp = str2double(datos{1});
            pm   = str2double(datos{2});
            accion = datos{3};
            
            if ~isnan(temp) && ~isnan(pm)
                muestra = muestra + 1;
                
                % Dibujar puntos en la gráfica
                addpoints(h_temp, muestra, temp);
                addpoints(h_pm, muestra, pm);
                
                
                fprintf('[Muestra %d] Temp: %d°C | PM: %d°C | Estado: %s\n', ...
                    muestra, temp, pm, accion);
                
                drawnow;
            end
        end
    catch
        disp('Desconectado o ventana cerrada.');
        break;
    end
end
