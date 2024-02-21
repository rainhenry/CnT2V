#!/usr/bin/python3
import gc
import torch
from diffusers import DiffusionPipeline, DPMSolverMultistepScheduler
from diffusers.utils import export_to_video


def pipe_init(model_id):
    gc.disable()
    pipe = DiffusionPipeline.from_pretrained(model_id, variant="fp16")
    pipe.scheduler = DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
    ##pipe.enable_vae_slicing()
    pipe = pipe.to('cpu')
    return pipe

def text_to_video(prompt, total_frames, output_file, pipe):
    gc.disable()
    video_frames = pipe(prompt, num_inference_steps=25, num_frames=total_frames).frames
    video_path = export_to_video(video_frames=video_frames[0], output_video_path=output_file)     
    return video_path


