from influxdb import DataFrameClient
from datetime import datetime
import pandas as pd

#quick script to pull your data from the database.

db_name = ""
measurement=""
client = DataFrameClient(database=db_name,host="XXX.XXX.XXX.XXX",username="USER",password="PASS",ssl=True,verify_ssl=False)

df = client.query("SELECT * FROM "+measurement)[measurement]

#default save name is the current time, plus database name and measurement name
save_name = datetime.now().strftime("%Y_%m_%dT%H_%M_%S_")+db_name + "_" + measurement+".csv"
print("Saving to "+save_name)
df.to_csv(save_name)