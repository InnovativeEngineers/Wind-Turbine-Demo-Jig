from os import system
import sys
import glob
import serial
import matplotlib.pyplot as plt
import matplotlib.colors as colors
import matplotlib.animation as animation
from matplotlib import style

## Serial Port Object
ser = serial.Serial()

## Data Storage
torque = [0]
rotation = [0]
wind = [0]

## Graphing Settings
REFRESH = 250
MAX_TORQUE = 50
MAX_ROTATION = 50
MAX_WIND = 25
GRID_STRENGTH = 0.2

## Graph Style/Theme
style.use('dark_background')

## Normalization For Mapping Data to Colormap
wind_norm = colors.Normalize(vmin = 0, vmax = MAX_WIND)

## Generates Figure Object with 4 Columns of Subplots (ax[0:3]) w/ ax[0] having 10x Width 
fig, ax = plt.subplots(nrows = 1, ncols = 4, gridspec_kw = {'width_ratios':[10,1,1,1]})

## Adjusts the Margins of the Figure and Space Between Subplots
plt.subplots_adjust(left = 0.075, right = 0.925, bottom = 0.075, top = 0.925, wspace = 0.3)

## "Plots" a Null Data Set to Initialize the Colormap
null_scatter = ax[0].scatter([],[], c = [], cmap = 'viridis', norm = wind_norm)

## Adds Colorbar to Subplot ax[0] based on null_scatter Colormapping
color_bar = fig.colorbar(null_scatter, ax = ax[0])
color_bar.ax.set_ylabel('Wind [m/s]')

## Adds Supertitle for the Entire Figure
fig.suptitle('BLADE CHARACTERIZATION')

## Main Function
def main():
    
    # mainloop
    while(1):
        
        # clear terminal
        system('clear')
        
        # print menu header
        print_header("DEMO JIG")
        
        # menu options
        print("1. RUN DEMO")
        
        # print current port
        print("2. PORT: {}".format(ser.port))
        
        # print current baudrate
        print("3. BAUDRATE: {}".format(ser.baudrate))
        
        # get user choice
        try:
            # cast to int from list
            user = int(input("\nCHOICE(#): "))
        except:
            print("\nINVALID ENTRY\n")
        else:
            # run the demo
            if user == 1:
                run_demo()
                
            # select serial port
            elif user == 2:
                select_port()
                
            # select baudrate
            elif user == 3:
                select_baudrate()
                
            # quit the process
            elif user == -1:
                system('clear')
                plt.close() # close figure
                exit() # exit process
                
            # otherwise...
            else:
                print("\nINVALID ENTRY\n")
    return

## Animation Function to Read and Plot Data in Realitme
def animate(i):
    
    # ensures global variables are used
    global torque, rotation, wind
    
    # check if the port's actually open
    if ser.isOpen() == False:
        return
            
    # try to read data from serial port
    try:
        
        # have to decode serial data from 'bytes' to 'ascii'
        data = ser.readline().decode('utf-8')
        
        # split data by commas
        values = data.split(',')
        
        # ensure valid data length
        if(len(values) == 3):
            
            # append data values to lists
            torque.append(float(values[0]))
            rotation.append(float(values[1]))
            wind.append(float(values[2]))
        
    # ignore error
    except:
        pass
    
    ## Scatter Plot
    ax[0].clear()
    
    # title subplot
    ax[0].set_title('Power Curves')
    
    # label x-axis
    ax[0].set_xlabel('Torque [N*m]')
    
    # set x-axis bounds
    ax[0].set_xlim(left = 0, right = MAX_TORQUE)
    
    # label y-axis
    ax[0].set_ylabel('Rotation [rad/s]')
    
    # set y-axis bounds
    ax[0].set_ylim(bottom = 0, top = MAX_ROTATION)
    
    # transparency of gridlines (0:off, 1:full)
    ax[0].grid(alpha = GRID_STRENGTH)
    
    # scatter(x_data, y_data, color_data, colormap, colormap_normalization)
    ax[0].scatter(torque, rotation, c = wind, cmap = 'viridis', norm = wind_norm)
    
    
    ## Torque Bar
    ax[1].clear()
    
    # title subplot 
    ax[1].set_title('Torque')
    
    # set y-axis bounds
    ax[1].set_ylim(bottom = 0, top = MAX_TORQUE)
    
    # bar(x_data, y_data)
    ax[1].bar('[N*m]', torque[-1])
    
    
    ## Rotation Bar
    ax[2].clear()
    
    # title subplot
    ax[2].set_title('Rotation')
    
    # set y-axis bounds
    ax[2].set_ylim(bottom = 0, top = MAX_ROTATION)
    
    # bar(x_data, y_data)
    ax[2].bar('[rad/s]', rotation[-1])
    
    
    ## Wind Bar
    ax[3].clear()
    
    # title subplot
    ax[3].set_title('Wind')
    
    # set y-axis bounds
    ax[3].set_ylim(bottom = 0, top = MAX_WIND)
    
    # bar(x_data, y_data)
    ax[3].bar('[m/s]', wind[-1])
    
    return

## Demo Runs Until User Presses Enter
def run_demo():
    global torque, rotation, wind
    system('clear')
    print_header('RUNNING DEMO')
    try:
        ser.open()
    except:
        print("\nUNABLE TO OPEN SERIAL PORT: {}\n".format(ser.port))
        return
    else:
        ani = animation.FuncAnimation(fig, animate, interval = REFRESH)
        plt.show(block = False)
        user = input('PRESS ENTER TO CLOSE')
    finally:
        # outputs data to csv file
        with open("data.csv", "w+") as file:
            for n in range(len(torque)):
                file.write("{}, {}, {}\n".format(torque[n], rotation[n], wind[n]))
        
        # reset globals
        torque = [0]
        rotation = [0]
        wind = [0]
        
        # close serail port
        ser.close()
    return

## Finds Available Serial Ports (from stackoverflow)
def serial_ports():
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i+1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported Platform')
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except(OSError, serial.SerialException):
            pass
    return result

## Lists & Selects Serial Port
def select_port():
    while(1):
        system('clear')
        print_header('PORTS')
        ports = serial_ports()
        for n in range(len(ports)):
            print(' {}. {}'.format(n+1, ports[n]))
        try:
            user = int(input('\nPORT(#): '))
        except:
            print('\nINVALID ENTRY\n')
        else:
            if user == -1:
                return
            elif 0 < user and user < (len(ports)+1):
                ser.port = ports[user-1]
                return
            else:
                print('\nINVALID ENTRY\n')
    return

## Lists & Selects Baudrate
def select_baudrate():
    baudrates = (300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 2880, 38400, 57600, 115200)
    while(1):
        system('clear')
        print_header('BAUDRATES')
        for n in range(len(baudrates)):
            print(' {}. {}'.format(n+1, baudrates[n]))
        try:
            user = int(input('\nBAUDRATE(#): '))
        except:
            print('\nINVALID ENTRY\n')
        else:
            if user == -1:
                return
            elif 0 < user and user < (len(baudrates)+1):
                ser.baudrate = baudrates[user-1]
                return
            elif (len(baudrates+1)) < user:
                ser.baudrate = user
                return
            else:
                print('\nINVALID ENTRY\n')
    return

## Fancy Menu Header Printing
def print_header(header):
    length = len(header)
    print('{}'.format('#'*(length+6)))
    print('## {} ##'.format(header))
    print('{}\n'.format('#'*(length+6)))
    return

## Runs "main()" Function when Script is Executed
if __name__ == '__main__':
    main()