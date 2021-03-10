import sys
from os import environ
from teslapy import Tesla

email     = environ.get('TESLA_EMAIL')
password  = environ.get('TESLA_PASSWORD')

if __name__ == "__main__":
    if not email:
        print("Tesla account email env var not set")
        exit()

    if not password:
        print("Tesla account password env var not set")
        exit()

    with Tesla(email, password) as tesla:
        tesla.fetch_token()
        selected = cars = tesla.vehicle_list()
        car = selected[0]

        if len(sys.argv) > 1 and int(sys.argv[1]) == 1:
            car.sync_wake_up()
            
            chg = car.get_vehicle_data()['charge_state']['battery_level']
            lon = car.get_vehicle_data()['drive_state']['longitude']
            lat = car.get_vehicle_data()['drive_state']['latitude']
            gear = car.get_vehicle_data()['drive_state']['shift_state']

            print(car.get_vehicle_data())

        else:
            print(car.get_vehicle_summary())
