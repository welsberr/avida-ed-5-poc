ARG EMSDK_IMAGE=emscripten/emsdk:6.0.10@sha256:e077d54e2b8970575ebc4f185ac1de0b95c05f2b266134d4ba27449af7aebf65
FROM ${EMSDK_IMAGE} AS web-build

WORKDIR /src/Avida
COPY . /src/Avida
RUN make web EMP_DIR=/src/Avida/vendor/Empirical CXX_web=em++ \
 && test -s web/Avida.js \
 && test -s web/Avida.wasm \
 && test -s web/Avida.data

FROM python:3.12-slim-bookworm@sha256:a116514e19457bcb7af7efe9c3dd0b9b71e85b317694e7882a1c52aa15a78134 AS web-runtime
WORKDIR /app
COPY --from=web-build /src/Avida/web /app
ENV HOST=0.0.0.0 PORT=8000
EXPOSE 8000
USER 65534:65534
CMD ["python3", "/app/serve.py"]
