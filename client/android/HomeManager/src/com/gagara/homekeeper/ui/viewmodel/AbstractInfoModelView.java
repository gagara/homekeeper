package com.gagara.homekeeper.ui.viewmodel;

import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.gagara.homekeeper.model.Model;

public abstract class AbstractInfoModelView<M extends Model> implements ModelView<M> {

    protected M model;
    protected View view;

    protected Resources resources;

    public AbstractInfoModelView() {
    }

    @Override
    public void saveState(Bundle bundle) {
        model.saveState(bundle);
    }

    @Override
    public void saveState(Bundle bundle, String prefix) {
        model.saveState(bundle, prefix);
    }

    @Override
    public void restoreState(Bundle bundle) {
        model.restoreState(bundle);
    }

    @Override
    public void restoreState(Bundle bundle, String prefix) {
        model.restoreState(bundle, prefix);

    }

    @Override
    public boolean isInitialized() {
        return model.isInitialized();
    }

    public M getModel() {
        return model;
    }

    public void setModel(M model) {
        this.model = model;
    }

    public View getView() {
        return view;
    }

    public void setView(TextView view) {
        this.view = view;
    }

    public Resources getResources() {
        return resources;
    }

    public void setResources(Resources resources) {
        this.resources = resources;
    }
}
