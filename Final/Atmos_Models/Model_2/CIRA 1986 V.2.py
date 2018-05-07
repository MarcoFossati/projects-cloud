#File location will need to be changed when downloaded
with open(r"C:\Users\Andrew\Documents\University\4th Year\Individual Project\PycharmProjects\Atmos_Models\Model_2\twp.lsn", "r") as searchfile:
    data = [x.strip().split() for x in searchfile.readlines()]

#Constants
R = 287.058

#Inputs
alt = float(raw_input('Geopotential Height (0 : 120km) > '))
hem = raw_input('Hemisphere (North : South) > ')
lat = float(raw_input('Latitude (0 : 80) > '))
mnt = raw_input('Month (Jan : Dec) > ')
print '\n\n'

#Defining starting points for line equations depending on month
if mnt == 'January' or mnt == 'january' or mnt == 'jan' or mnt == 'Jan' or mnt == '1':
    c1 = 30
    c2 = 62
    c3 = 94
elif mnt == 'February' or mnt == 'february' or mnt == 'feb' or mnt == 'Feb' or mnt == '2':
    c1 = 127
    c2 = 159
    c3 = 191

elif mnt == 'March' or mnt == 'march' or mnt =='mar' or mnt == 'Mar' or mnt == '3':
    c1 = 224
    c2 = 256
    c3 = 288

elif mnt == 'April' or mnt == 'april' or mnt =='apr' or mnt == 'Apr' or mnt == '4':
    c1 = 321
    c2 = 353
    c3 = 385

elif mnt == 'May' or mnt == 'may' or mnt == '5':
    c1 = 418
    c2 = 450
    c3 = 482

elif mnt == 'June' or mnt == 'june' or mnt == 'Jun' or mnt == 'jun' or mnt == '6':
    c1 = 515
    c2 = 547
    c3 = 579

elif mnt == 'July' or mnt == 'july' or mnt == 'Jul' or mnt == 'jul' or mnt == '7':
    c1 = 612
    c2 = 644
    c3 = 676

elif mnt == 'August' or mnt == 'august' or mnt == 'Aug' or mnt == 'aug' or mnt == '8':
    c1 = 709
    c2 = 741
    c3 = 773

elif mnt == 'September' or mnt == 'september' or mnt == 'Sep' or mnt == 'sep' or mnt == '9':
    c1 = 806
    c2 = 838
    c3 = 870

elif mnt == 'October' or mnt == 'october' or mnt == 'oct' or mnt == 'Oct' or mnt == '10':
    c1 = 903
    c2 = 935
    c3 = 967

elif mnt == 'November' or mnt == 'november' or mnt == 'Nov' or mnt == 'nov' or mnt == '11':
    c1 = 1000
    c2 = 1032
    c3 = 1064

elif mnt == 'December' or mnt == 'december' or mnt == 'Dec' or mnt == 'dec' or mnt == '12':
    c1 = 1097
    c2 = 1129
    c3 = 1161

altt = -0.2*alt + c1 #Line number of raw altitude input for temperature
altp = -0.2*alt + c3 #Line number of raw altitude input for pressure
altw = -0.2*alt + c2 #Line number of raw altitude input for wind

#Selecting min and max columns to use to interpolate for given latitude in southern hemisphere
if hem == 'South' or hem == 'south' or hem == 'S' or hem == 's':
    latl = -0.1*lat + 9
    max_col = int(-0.1 * (int(float(lat)/2)*2) + 9)
    tap = (-0.1 * ((float(lat)/2)*2) + 9)
    if max_col > tap: #Anomaly with standard method when using specific values from tables e.g. 61, 60.4, 10.2, 65.2, 95.1, 106
        max_col = max_col - 1
    min_col = max_col + 1

#Selecting min and max columns to use to interpolate for given latitude in northern hemisphere
elif hem == 'North' or hem == 'north' or hem == 'N' or hem == 'n':
    latl = 0.1*lat + 9
    min_col = int(0.1*(int(float(lat)/2)*2) + 9)
    hit = (0.1 * ((float(lat)/2)*2) + 9)
    if min_col > hit: # Same method and anomaly
        min_col = min_col - 1
    max_col = min_col + 1
    if lat == 80:
        min_col = 16
        max_col = 17

#Defining min and max temperature data to interpolate
templ_max = int(-0.2 * (int(float(alt)/2)*2) + c1)
check = (-0.2 * ((float(alt)/2)*2) + c1)
if templ_max > check: # Same method and anomaly
    templ_max = templ_max - 1
templ_min = templ_max + 1

#Defining min and max pressure data to interpolate using same method
pressl_max = int(-0.2 * (int(float(alt)/2)*2) + c3)
tick = (-0.2 * ((float(alt)/2)*2) + c3)
if pressl_max > tick: # Same method and anomaly
    pressl_max = pressl_max - 1
pressl_min = pressl_max + 1

#Defining min and max wind speed data to interpolate using same method
windl_max = int(-0.2 * (int(float(alt)/2)*2) + c2)
tock = (-0.2 * ((float(alt)/2)*2) + c2)
if windl_max > tock: # Same method and anomaly
    windl_max = windl_max - 1
windl_min = windl_max + 1

#Everything up to this point has been selecting the min and max lines and columns for the properties from that inputs given.
#From this point down, the line numbers and columns calculated will be used to call data from the table.

min_temp_min = float(data[templ_min][min_col]) #Temperature and minimum altitude and minimum latitude
max_temp_min = float(data[templ_max][min_col]) #Temperature at maximum altitude and minimum latitude
min_temp_max = float(data[templ_min][max_col]) #Temperature at minimum altitude and maximum latitude
max_temp_max = float(data[templ_max][max_col]) #Temperature at maximum altitude and maximum latitude

min_wind_min = float(data[windl_min][min_col])
max_wind_min = float(data[windl_max][min_col])
min_wind_max = float(data[windl_min][max_col])
max_wind_max = float(data[windl_max][max_col])

temp_min = min_temp_min + ((max_temp_min - min_temp_min)*(altt - templ_min))/(templ_max - templ_min)
temp_max = max_temp_min + ((max_temp_max - max_temp_min)*(altt - templ_min))/(templ_max - templ_min)
temp = temp_min + ((temp_max - temp_min)*(latl - min_col))/(max_col - min_col)

wind_min = min_wind_min + ((max_wind_min - min_wind_min)*(altw - windl_min))/(windl_max - windl_min)
wind_max = max_wind_min + ((max_wind_max - max_wind_min)*(altw - windl_min))/(windl_max - windl_min)
wind = wind_min + ((wind_max - wind_min)*(latl - min_col))/(max_col - min_col)

if alt >= 20:
    min_press_min = 100 * float(data[pressl_min][min_col])*float(data[pressl_min][18])
    max_press_min = 100 * float(data[pressl_max][min_col])*float(data[pressl_max][18])
    min_press_max = 100 * float(data[pressl_min][max_col])*float(data[pressl_min][18])
    max_press_max = 100 * float(data[pressl_max][max_col])*float(data[pressl_max][18])

    press_min = min_press_min + ((max_press_min - min_press_min)*(altp - pressl_min))/(pressl_max - pressl_min)
    press_max = max_press_min + ((max_press_max - max_press_min)*(altp - pressl_min))/(pressl_max - pressl_min)
    press = press_min + ((press_max - press_min)*(latl - min_col))/(max_col - min_col)

    dens = press / (R*temp)

elif alt < 20:
    press = 'N/A'
    dens = 'N/A'

if alt > 86:
    print 'NOTE: [Density calculated as perfect gas may be inaccurate above 86km]\n'

print "Altitude:",alt,"km",'\n','-'*25,"\nTemperature:",temp,"K","\nPressure:",press,"Pa","\nDensity:",round(dens,9),"kg/m3"\
    "\nZonal Wind Speed:",wind,"m/s",'\n','-'*25