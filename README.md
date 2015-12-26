# EZTemp

Twig like templating library and tools.

## Requirements

This library is on top of the *boost* library.

On debian-based distro:

```bash
sudo apt-get install \
    libboost-program-options-dev \
    libboost-python-dev \
    libboost-regex-dev \
    libboost-date-time-dev
```

## Installation

```bash
git clone https://github.com/Garcia6l20/eztemp
cd eztemp
mkdir build
cd build
cmake ..
make

# run some tests
make check

# install
sudo make install
```

## Usage

### In code

For in code examples check the `examples/helloworld` example.

### Compiler

- `layout.txt.ez`:

```txt
Header:
{% block header %}
This is my default header.
{% endblock %}

Content:
{% block content %}
This is my default content.
{% endblock %}
```

- `template.txt.ez`:

```txt
{% extends layout.txt %}
{% block content %}
My actual content:
{% for item in items %}
  - {{ loop.index }}: {{ item }} {% if loop.last %} !!!{% endif %}
{% endfor %}
{% endblock %}
```

- `params.json`:

```json
{
  "items": [ "Big", "Bad", "Wolf" ]
}
```

Then run:

```bash
eztemp-cc template.txt.ez -p params.json out.txt
```

- `out.txt`:

```txt
Header:
This is my default header.

Content:
My actual content:
  - 1: Big
  - 2: Bad
  - 3: Wolf  !!!
```
