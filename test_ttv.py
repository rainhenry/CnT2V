#!/usr/bin/python3
import torch
from diffusers import DiffusionPipeline, DPMSolverMultistepScheduler
from diffusers.utils import export_to_video
pipe = DiffusionPipeline.from_pretrained("./text-to-video-ms-1.7b", variant="fp16")
pipe.scheduler = DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
##pipe.enable_vae_slicing()
pipe = pipe.to('cpu')
prompt = "Spiderman is surfing"
video_frames = pipe(prompt, num_inference_steps=25, num_frames=16).frames
video_path = export_to_video(video_frames=video_frames[0], output_video_path='test.mp4')     

