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
        try:
            tesla.fetch_token()
            selected = cars = tesla.vehicle_list()
            car = selected[0]

            car.sync_wake_up()

            state = car.get_vehicle_summary()['state']
           
            if state == "online":
                print(1)

            else: 
                print(0)

        except:
            print(-1)