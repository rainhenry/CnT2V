#!/usr/bin/python3
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM
tokenizer = AutoTokenizer.from_pretrained("./opus-mt-zh-en")
model = AutoModelForSeq2SeqLM.from_pretrained("./opus-mt-zh-en")
in_text='猫在爬树'
inputs = tokenizer(in_text, return_tensors='pt')
pred = model.generate(**inputs)
output = tokenizer.decode(pred.cpu()[0], skip_special_tokens=True)
print(output)
