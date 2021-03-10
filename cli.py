""" Tesla Owner API CLI application using TeslaPy module """

# Author: Tim Dorssers

from __future__ import print_function
import logging
import getpass
import argparse
from teslapy import Tesla

raw_input = vars(__builtins__).get('raw_input', input)  # Py2/3 compatibility

def get_passcode(args):

    return args.passcode if args.passcode else raw_input('Passcode: ')

def main():
    parser = argparse.ArgumentParser(description='Tesla Owner API CLI')
    parser.add_argument('-e', dest='email', help='login email', required=True)
    parser.add_argument('-p', dest='password', nargs='?', const='',
                        help='prompt/specify login password')
    parser.add_argument('-t', dest='passcode', help='two factor passcode')
    parser.add_argument('-u', dest='factor', help='use two factor device name')
    parser.add_argument('-f', dest='filter', help='filter on id, vin, etc.')
    parser.add_argument('-a', dest='api', help='API call endpoint name')
    parser.add_argument('-k', dest='keyvalue', help='API parameter (key=value)',
                        action='append', type=lambda kv: kv.split('=', 1))
    parser.add_argument('-c', dest='command', help='vehicle command endpoint')
    parser.add_argument('-l', '--list', action='store_true',
                        help='list all selected vehicles')
    parser.add_argument('-o', '--option', action='store_true',
                        help='list vehicle option codes')
    parser.add_argument('-v', '--vin', action='store_true',
                        help='vehicle identification number decode')
    parser.add_argument('-w', '--wake', action='store_true',
                        help='wake up selected vehicle(s)')
    parser.add_argument('-g', '--get', action='store_true',
                        help='get rollup of all vehicle data')
    parser.add_argument('-n', '--nearby', action='store_true',
                        help='list nearby charging sites')
    parser.add_argument('-m', '--mobile', action='store_true',
                        help='get mobile enabled state')
    parser.add_argument('-s', '--start', action='store_true',
                        help='remote start drive')
    parser.add_argument('-d', '--debug', action='store_true',
                        help='set logging level to debug')
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO,
                        format='%(asctime)s - %(levelname)s - %(message)s')
    if args.password == '':
        password = getpass.getpass('Password: ')
    else:
        password = args.password
    with Tesla(args.email, password, lambda: get_passcode(args),
               (lambda _: args.factor) if args.factor else None) as tesla:
        tesla.fetch_token()
        selected = cars = tesla.vehicle_list()
        if args.filter:
            selected = [c for c in cars for v in c.values() if v == args.filter]
        logging.info('%d vehicle(s), %d selected', len(cars), len(selected))
        for i, vehicle in enumerate(selected):
            print('Vehicle %d:' % i)
            if args.list:
                print(vehicle)
            if args.option:
                print(', '.join(vehicle.option_code_list()))
            if args.vin:
                print(vehicle.decode_vin())
            if args.wake:
                vehicle.sync_wake_up()
            if args.get:
                print(vehicle.get_vehicle_data())
            if args.nearby:
                print(vehicle.get_nearby_charging_sites())
            if args.mobile:
                print(vehicle.mobile_enabled())
            if args.start:
                print(vehicle.remote_start_drive())
            if args.api:
                data = dict(args.keyvalue) if args.keyvalue else {}
                print(vehicle.api(args.api, **data))

if __name__ == "__main__":
    main()
