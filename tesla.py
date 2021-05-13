import sys
from os import environ
from teslapy import Tesla, Vehicle

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
        try:
            tesla.fetch_token()
            selected = cars = tesla.vehicle_list()
            car = selected[0]

            state = car.get_vehicle_summary()['state']
           
            if state == "online":
                print(car.get_vehicle_data())

            else: 
                print('{"online": 0}')

        except:
            print('{"request-error": 1}')
