FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY bridge.py .
VOLUME /data
EXPOSE 1883
CMD ["python", "-u", "bridge.py"]
