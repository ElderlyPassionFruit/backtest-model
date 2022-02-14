import pickle
import json

file_name = input()

input_file = file_name + ".pkl"
output_file = file_name + ".csv"

with open(input_file, "rb") as f:
    print(pickle.load(f).to_csv(), file = open(output_file, "w"))
