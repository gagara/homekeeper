package com.gagara.homekeeper.ui.viewmodel;

import java.util.HashSet;
import java.util.Set;

import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.gagara.homekeeper.model.Model;

public abstract class AbstractEntryModelView implements ModelView {

    protected Model model;
    protected View valueView;
    protected View detailsView;

    protected Resources resources;

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

    public Model getModel() {
        return model;
    }

    public Set<View> getViews() {
        Set<View> result = new HashSet<View>();
        result.add(valueView);
        result.add(detailsView);
        return result;
    }

    public View getValueView() {
        return valueView;
    }

    public View getDetailsView() {
        return detailsView;
    }

    public void setModel(Model model) {
        this.model = model;
    }

    public void setValueView(TextView valueView) {
        this.valueView = valueView;
    }

    public void setDetailsView(TextView detailsView) {
        this.detailsView = detailsView;
    }

    public Resources getResources() {
        return resources;
    }

    public void setResources(Resources resources) {
        this.resources = resources;
    }
}
