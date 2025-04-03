## How to build

1. Install build dependencies: https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies
2. Clone repo and install project dependencies:
```bash
git clone https://github.com/buglloc/h4ptix.git
cd h4ptix
pip install pipenv
pipenv install --dev
pipenv shell
cd firmware
west packages pip --install
west update
```
3. Build:
```bash

west build -b h4ptix/rp2040/display app
```

Available boards:
  - `h4ptix/rp2040/lonely` - lonely board
  - `h4ptix/rp2040/basic` - basic board
  - `h4ptix/rp2040/display` - basic + display
