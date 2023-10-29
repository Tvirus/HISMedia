
products_list_hi3516dv500 :=
products_list_hi3516dv500 += hi3516dv500

products_list_t41 :=
products_list_t41 += t41nq

products_list_ssc377 :=
products_list_ssc377 += ssc377de


.PHONY: help
.PHONY: $(products_list_hi3516dv500)
.PHONY: $(products_list_t41)
.PHONY: $(products_list_ssc377)

help:
	@echo "Usage: make <product>"
	@exit 1

$(products_list_hi3516dv500):
	make -C ./src/hi3516dv500 PRODUCT=$@

$(products_list_t41):
	make -C ./src/t41 PRODUCT=$@

$(products_list_ssc377):
	make -C ./src/ssc377 PRODUCT=$@
