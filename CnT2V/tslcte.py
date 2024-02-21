#!/usr/bin/python3
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM


def model_init(model_id):
    model = AutoModelForSeq2SeqLM.from_pretrained(model_id)
    return model

def tokenizer_init(model_id):
    tokenizer = AutoTokenizer.from_pretrained(model_id)
    return tokenizer

def translate_c2e(in_text, model, tokenizer):
    inputs = tokenizer(in_text, return_tensors='pt')
    pred = model.generate(**inputs)
    output = tokenizer.decode(pred.cpu()[0], skip_special_tokens=True)
    return output

