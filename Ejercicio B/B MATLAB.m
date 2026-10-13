
clear; clc; close all;

puertoCOM = 'COM3'; 
baudios = 9600;

try
    s = serialport(puertoCOM, baudios);
    configureTerminator(s, "CR/LF");
    flush(s);
    disp('Conexión exitosa con el ATmega328P. Graficando telemetría...');
catch ME
    error('Error al abrir el puerto serie. Cierra la consola de Microchip Studio/Arduino e intenta de nuevo.');
end

fig = figure('Name', 'Sistema de Control de Iluminación - Telemetría Integrada', ...
             'NumberTitle', 'off', 'Color', [0.97 0.97 0.97], 'Position', [100 100 1000 600]);

ax = axes('Parent', fig);
hold(ax, 'on');
grid(ax, 'on');
box(ax, 'on');

title(ax, 'Telemetría en Tiempo Real - Control de Iluminación y Persiana Motorizada', 'FontSize', 14);
xlabel(ax, 'Muestras de Tiempo', 'FontSize', 11);
ylabel(ax, 'Magnitud / Lectura ADC', 'FontSize', 11);
ylim(ax, [0 1080]); 

h_sp  = animatedline('Color', [0.85 0.00 0.00], 'LineWidth', 2.0, ...   
                     'DisplayName', 'ROJO: Setpoint (SP) - Nivel de Luz Deseado [0-1023]');
                     
h_ldr = animatedline('Color', [0.00 0.45 0.85], 'LineWidth', 1.8, ...   
                     'DisplayName', 'AZUL: Luz Real (LDR) - Iluminación Medida [0-1023]');
                     
h_pot = animatedline('Color', [0.10 0.65 0.20], 'LineWidth', 1.8, ...   
                     'DisplayName', 'VERDE: Posición (POT) - Ángulo del Mecanismo [0-1023]');
                     
h_pwm = animatedline('Color', [0.90 0.50 0.00], 'LineWidth', 1.5, ...   
                     'DisplayName', 'NARANJA: Velocidad (PWM) - Esfuerzo de Control [0-255]');

lgd = legend(ax, 'Location', 'northeastoutside', 'FontSize', 10);
title(lgd, 'Código de Colores / Variables');

maxMuestras = 100; 
t = 0;

cleanupObj = onCleanup(@() clear('s'));

while ishandle(fig)
    if s.NumBytesAvailable > 0
        linea = readline(s);
        
        datos = sscanf(linea, 'SP:%d | LDR:%d | POT:%d | PWM:%d');
        
        if length(datos) == 4
            sp  = datos(1);
            ldr = datos(2);
            pot = datos(3);
            pwm = datos(4);
            
            t = t + 1;
            
            addpoints(h_sp,  t, sp);
            addpoints(h_ldr, t, ldr);
            addpoints(h_pot, t, pot);
            addpoints(h_pwm, t, pwm);
            
            if t > maxMuestras
                xlim(ax, [t - maxMuestras, t]);
            end
            
            drawnow limitrate;
        end
    end
end