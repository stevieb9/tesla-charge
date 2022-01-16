import sys
from os import environ
import teslapy

email     = environ.get('TESLA_EMAIL')
password  = environ.get('TESLA_PASSWORD')

if __name__ == "__main__":
    if not email:
        print("Tesla account email env var not set")
        exit()

    if not password:
        print("Tesla account password env var not set")
        exit()

    with teslapy.Tesla(email) as tesla:
        try:
#            tesla.fetch_token()
            selected = cars = tesla.vehicle_list()
            car = selected[0]

            state = car.get_vehicle_summary()['state']
           
            if state == "online":
                print(car.get_vehicle_data())

            else: 
                print('{"online": 0}')

        except:
            print('{"request-error": 1}')
